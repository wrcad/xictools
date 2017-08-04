
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

#include "fio.h"
#include "fio_oasis.h"
#include "fio_oas_sort.h"
#include "fio_oas_reps.h"
#include "hashfunc.h"


//---------------------------------------------------------------------------
// Object cache and repetition recognizer
//
// The following code implements a cache for objects and instances, and
// processing to optimize the ordering and to recognize repetitions.
//---------------------------------------------------------------------------

// These functions are used to add objects to the cache.  One copy of each
// unique object is saved in memory, with a list of points to the locations
// of identical objects.


// The rep_args is the value of the OasWriteRep variable.  If null,
// no repetitions caching, if empty use default caching, else parse
// the value and set things accordingly.  The second arg is the
// value of the OasWriteNoGCDcheck variable.
//
void
oas_cache::setup_repetition(const char *rep_args, bool no_gcd_check)
{
    oc_cache_max = REP_MAX_ITEMS;
    oc_rep_max = REP_MAX_REPS;
    oc_minrun = REP_RUN_MIN;
    oc_minarr = REP_ARRAY_MIN;
    oc_random_placement = false;
    oc_rep_debug = false;
    oc_gcd_check = !no_gcd_check;
    oc_repetitions = 0;

    const char *s = rep_args;
    if (!s)
        // variable undefined, no caching
        return;

    bool set = false;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {

        if (*tok == 'r') {
            // namdom repetition only, no runs or arrays
            oc_random_placement = true;
        }
        else if (*tok == 'm') {
            char *t = strchr(tok, '=');
            if (t) {
                t++;
                int n = atoi(t);
                if (n >= REP_RUN_MIN && n <= REP_RUN_MAX)
                    oc_minrun = n;
                else
                    FIO()->ifPrintCvLog(IFLOG_WARN,
                        "OasWriteRep value for m is bad, default used.");
            }
            else
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "OasWriteRep bad syntax for m, no '=', default used.");
        }
        else if (*tok == 'a') {
            char *t = strchr(tok, '=');
            if (t) {
                t++;
                int n = atoi(t);
                if (n == 0 || (n >= REP_ARRAY_MIN && n <= REP_ARRAY_MAX))
                    oc_minarr = n;
                else
                    FIO()->ifPrintCvLog(IFLOG_WARN,
                        "OasWriteRep value for a is bad, default used.");
            }
            else
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "OasWriteRep bad syntax for a, no '=', default used.");
        }
        else if (*tok == 't') {
            char *t = strchr(tok, '=');
            if (t) {
                t++;
                int n = atoi(t);
                if (n == 0 || (n >= REP_MAX_REPS_MIN && n <= REP_MAX_REPS_MAX))
                    oc_rep_max = n;
                else
                    FIO()->ifPrintCvLog(IFLOG_WARN,
                        "OasWriteRep value for t is bad, default used.");
            }
            else
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "OasWriteRep bad syntax for t, no '=', default used.");
        }
        else if (*tok == 'd') {
            // debugging enabled
            oc_rep_debug = true;
        }
        else if (*tok == 'x') {
            char *t = strchr(tok, '=');
            if (t) {
                t++;
                int n = atoi(t);
                if (n >= REP_MAX_ITEMS_MIN && n <= REP_MAX_ITEMS_MAX)
                    oc_cache_max = n;
                else
                    FIO()->ifPrintCvLog(IFLOG_WARN,
                        "OasWriteRep value for x is bad, default used.");
            }
            else
                FIO()->ifPrintCvLog(IFLOG_WARN,
                    "OasWriteRep bad syntax for x, no '=', default used.");
        }
        else {
            if (strchr(tok, 'b'))
                oc_repetitions |= OAS_CR_BOX;
            if (strchr(tok, 'p'))
                oc_repetitions |= OAS_CR_POLY;
            if (strchr(tok, 'w'))
                oc_repetitions |= OAS_CR_WIRE;
            if (strchr(tok, 'l'))
                oc_repetitions |= OAS_CR_LAB;
            if (strchr(tok, 'c'))
                oc_repetitions |= OAS_CR_CELL;
            set = true;
        }
        delete [] tok;
    }
    if (!set) {
        // defined as bool, cache everything
        oc_repetitions = OAS_CR_GEOM | OAS_CR_CELL;
        return;
    }
}


bool
oas_cache::cache_box(const BBox *BB, unsigned int l, unsigned int d,
    CDp *props)
{
    if (!oc_box_tab)
        oc_box_tab = new rpdb_t<rf_box>;
    rf_box b;
    b.set(BB, l, d, props);

    rf_box *db = oc_box_tab->record(&b);
    if (!db->others) {
        if (b.x != db->x || b.y != db->y)
            db->others = new_pt(db->x, db->y, 0);
        else {
            if (b.properties == db->properties)
                b.properties = 0;
        }
    }
    if (db->others) {
        db->others = new_pt(b.x, b.y, db->others);
        oc_box_rep_cnt++;
    }
    b.cleanup();
    return (true);
}


bool
oas_cache::cache_poly(const Poly *po, unsigned int l, unsigned int d,
    CDp *props)
{
    if (!oc_poly_tab)
        oc_poly_tab = new rpdb_t<rf_poly>;
    rf_poly p;
    p.set(po, l, d, props);

    rf_poly *dp = oc_poly_tab->record(&p);
    if (!dp->boxpart.others) {
        if (p.boxpart.x != dp->boxpart.x || p.boxpart.y != dp->boxpart.y)
            dp->boxpart.others = new_pt(dp->boxpart.x, dp->boxpart.y, 0);
        else {
            if (p.boxpart.properties == dp->boxpart.properties)
                p.boxpart.properties = 0;
            if (p.points == dp->points)
                p.points = 0;
        }
    }
    if (dp->boxpart.others) {
        dp->boxpart.others =
            new_pt(p.boxpart.x, p.boxpart.y, dp->boxpart.others);
        oc_poly_rep_cnt++;
    }
    p.cleanup();
    return (true);
}


bool
oas_cache::cache_wire(const Wire *wo, unsigned int l, unsigned int d,
    CDp *props)
{
    if (!oc_wire_tab)
        oc_wire_tab = new rpdb_t<rf_wire>;
    rf_wire w;
    w.set(wo, l, d, props);

    rf_wire *dw = oc_wire_tab->record(&w);
    if (!dw->boxpart.others) {
        if (w.boxpart.x != dw->boxpart.x || w.boxpart.y != dw->boxpart.y)
            dw->boxpart.others = new_pt(dw->boxpart.x, dw->boxpart.y, 0);
        else {
            if (w.boxpart.properties == dw->boxpart.properties)
                w.boxpart.properties = 0;
            if (w.points == dw->points)
                w.points = 0;
        }
    }
    if (dw->boxpart.others) {
        dw->boxpart.others =
            new_pt(w.boxpart.x, w.boxpart.y, dw->boxpart.others);
        oc_wire_rep_cnt++;
    }
    w.cleanup();
    return (true);
}


bool
oas_cache::cache_label(const Text *tx, unsigned int l, unsigned int d,
    CDp *props)
{
    if (!oc_label_tab)
        oc_label_tab = new rpdb_t<rf_label>;
    rf_label s;
    s.set(tx, l, d, props);

    rf_label *ds = oc_label_tab->record(&s);
    if (!ds->boxpart.others) {
        if (s.boxpart.x != ds->boxpart.x || s.boxpart.y != ds->boxpart.y)
            ds->boxpart.others = new_pt(ds->boxpart.x, ds->boxpart.y, 0);
        else {
            if (s.boxpart.properties == ds->boxpart.properties)
                s.boxpart.properties = 0;
            if (s.label == ds->label)
                s.label = 0;
        }
    }
    if (ds->boxpart.others) {
        ds->boxpart.others =
            new_pt(s.boxpart.x, s.boxpart.y, ds->boxpart.others);
        oc_label_rep_cnt++;
    }
    s.cleanup();
    return (true);
}


bool
oas_cache::cache_sref(const Instance *inst, CDp *props)
{
    if (!oc_cname_tab)
        oc_cname_tab = new strtab_t;
    const char *name = oc_cname_tab->add(inst->name);
    if (!oc_sref_tab)
        oc_sref_tab = new rpdb_t<rf_sref>;
    rf_sref c;
    c.set(inst, name, props);

    rf_sref *dc = oc_sref_tab->record(&c);
    if (!dc->others) {
        if (c.tx.tx != dc->tx.tx || c.tx.ty != dc->tx.ty)
            dc->others = new_pt(dc->tx.tx, dc->tx.ty, 0);
        else {
            if (c.properties == dc->properties)
                c.properties = 0;
            if (c.cname == dc->cname)
                c.cname = 0;
        }
    }
    if (dc->others) {
        dc->others = new_pt(c.tx.tx, c.tx.ty, dc->others);
        oc_sref_rep_cnt++;
    }
    c.cleanup();
    return (true);
}


// These functions flush the cache.  The objects are sorted, repetitions
// are found, and the output is generated through oas_out.

bool
oas_cache::flush()
{
    if (!flush_box())
        return (false);
    if (!flush_poly())
        return (false);
    if (!flush_wire())
        return (false);
    if (!flush_label())
        return (false);
    if (!flush_sref())
        return (false);

    oc_pt_factory.clear();
    return (true);
}


bool
oas_cache::flush_box()
{
    bool ret = true;
    if (oc_box_tab) {
        if (oc_rep_debug)
            printf("** flush box\n");
        rf_box **ary = oc_box_tab->extract_array();
        if (ary) {
            unsigned int count = oc_box_tab->allocated();
            sorter<rf_box> sa(ary, count, oc_out->out_faster_sort);
            sa.sort();
            for (unsigned int i = 0; i < count; i++) {
                ret = write_box_cache(ary[i]);
                if (!ret)
                    break;
            }
            delete [] ary;
        }
        delete oc_box_tab;
        oc_box_tab = 0;
        oc_box_rep_cnt = 0;
    }
    return (ret);
}


bool
oas_cache::flush_poly()
{
    bool ret = true;
    if (oc_poly_tab) {
        if (oc_rep_debug)
            printf("** flush poly\n");
        rf_poly **ary = oc_poly_tab->extract_array();
        if (ary) {
            unsigned int count = oc_poly_tab->allocated();
            sorter<rf_poly> sa(ary, count, oc_out->out_faster_sort);
            sa.sort();
            for (unsigned int i = 0; i < count; i++) {
                ret = write_poly_cache(ary[i]);
                if (!ret)
                    break;
            }
            delete [] ary;
        }
        delete oc_poly_tab;
        oc_poly_tab = 0;
        oc_poly_rep_cnt = 0;
    }
    return (ret);
}


bool
oas_cache::flush_wire()
{
    bool ret = true;
    if (oc_wire_tab) {
        if (oc_rep_debug)
            printf("** flush wire\n");
        rf_wire **ary = oc_wire_tab->extract_array();
        if (ary) {
            unsigned int count = oc_wire_tab->allocated();
            sorter<rf_wire> sa(ary, count, oc_out->out_faster_sort);
            sa.sort();
            for (unsigned int i = 0; i < count; i++) {
                ret = write_wire_cache(ary[i]);
                if (!ret)
                    break;
            }
            delete [] ary;
        }
        delete oc_wire_tab;
        oc_wire_tab = 0;
        oc_wire_rep_cnt = 0;
    }
    return (ret);
}


bool
oas_cache::flush_label()
{
    bool ret = true;
    if (oc_label_tab) {
        if (oc_rep_debug)
            printf("** flush label\n");
        rf_label **ary = oc_label_tab->extract_array();
        if (ary) {
            unsigned int count = oc_label_tab->allocated();
            sorter<rf_label> sa(ary, count, oc_out->out_faster_sort);
            sa.sort();
            for (unsigned int i = 0; i < count; i++) {
                ret = write_label_cache(ary[i]);
                if (!ret)
                    break;
            }
            delete [] ary;
        }
        delete oc_label_tab;
        oc_label_tab = 0;
        oc_label_rep_cnt = 0;
    }
    return (ret);
}


bool
oas_cache::flush_sref()
{
    bool ret = true;
    if (oc_sref_tab) {
        if (oc_rep_debug)
            printf("** flush sref\n");
        rf_sref **ary = oc_sref_tab->extract_array();
        if (ary) {
            unsigned int count = oc_sref_tab->allocated();
            sorter<rf_sref> sa(ary, count, oc_out->out_faster_sort);
            sa.sort();
            for (unsigned int i = 0; i < count; i++) {
                ret = write_sref_cache(ary[i]);
                if (!ret)
                    break;
            }
            delete [] ary;
        }
        delete oc_sref_tab;
        oc_sref_tab = 0;
        oc_cname_tab->clear();
        oc_sref_rep_cnt = 0;
    }
    return (ret);
}


// These functions write the objects from the cache.

bool
oas_cache::write_cached_box(const rf_box *b, int x, int y)
{
    bool ret = oc_out->setup_properties(b->properties);
    if (ret) {
        BBox BB(x, y, x + b->width, y + b->height);
        oc_out->out_cache = 0;
        ret = oc_out->write_box(&BB);
        oc_out->out_cache = this;
    }
    oc_out->clear_property_queue();
    return (ret);
}


bool
oas_cache::write_box_cache(const rf_box *b)
{
    int tmp_ly, tmp_dt;
    oc_out->set_layer_dt(b->layer, b->datatype, &tmp_ly, &tmp_dt);

    bool ret = true;
    coor_t *rpts = b->others;
    if (!rpts)
        ret = write_cached_box(b, b->x, b->y);
    else {
        reps_t r(oc_minrun, oc_minarr, rpts);
        r.build();

        for (const twod_t *t = r.x_array_list(); t && ret; t = t->next) {
            set_repetition(t);
            ret = write_cached_box(b, t->eltx, t->elty);
            unset_repetition();
        }
        for (const twod_t *t = r.y_array_list(); t && ret; t = t->next) {
            set_repetition(t);
            ret = write_cached_box(b, t->eltx, t->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.x_run_list(); o && ret; o = o->next) {
            set_repetition(o);
            ret = write_cached_box(b, o->eltx, o->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.y_run_list(); o && ret; o = o->next) {
            set_repetition(o);
            ret = write_cached_box(b, o->eltx, o->elty);
            unset_repetition();
        }
        if (r.point_list() && ret) {
            set_repetition(r.point_list());
            ret = write_cached_box(b, r.point_list()->x, r.point_list()->y);
            unset_repetition();
        }
    }

    oc_out->set_layer_dt(tmp_ly, tmp_dt);
    return (ret);
}


bool
oas_cache::write_cached_poly(const rf_poly *p, int x, int y)
{
    bool ret = oc_out->setup_properties(p->boxpart.properties);
    if (ret) {
        Poly po;
        po.numpts = p->numpts + 2;
        po.points = new Point[po.numpts];
        po.points[0].set(x, y);
        int i;
        for (i = 0; i < p->numpts; i++)
            po.points[i+1].set(p->points[i].x + po.points[0].x,
                p->points[i].y + po.points[0].y);
        po.points[i+1] = po.points[0];
        oc_out->out_cache = 0;
        ret = oc_out->write_poly(&po);
        oc_out->out_cache = this;
        delete [] po.points;
    }
    oc_out->clear_property_queue();
    return (ret);
}


bool
oas_cache::write_poly_cache(const rf_poly *p)
{
    int tmp_ly, tmp_dt;
    oc_out->set_layer_dt(p->boxpart.layer, p->boxpart.datatype,
        &tmp_ly, &tmp_dt);

    bool ret = true;
    coor_t *rpts = p->boxpart.others;
    if (!rpts)
        ret = write_cached_poly(p, p->boxpart.x, p->boxpart.y);
    else {
        reps_t r(oc_minrun, oc_minarr, rpts);
        r.build();

        for (const twod_t *t = r.x_array_list(); t && ret; t = t->next) {
            set_repetition(t);
            ret = write_cached_poly(p, t->eltx, t->elty);
            unset_repetition();
        }
        for (const twod_t *t = r.y_array_list(); t && ret; t = t->next) {
            set_repetition(t);
            ret = write_cached_poly(p, t->eltx, t->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.x_run_list(); o && ret; o = o->next) {
            set_repetition(o);
            ret = write_cached_poly(p, o->eltx, o->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.y_run_list(); o && ret; o = o->next) {
            set_repetition(o);
            ret = write_cached_poly(p, o->eltx, o->elty);
            unset_repetition();
        }
        if (r.point_list() && ret) {
            set_repetition(r.point_list());
            ret = write_cached_poly(p, r.point_list()->x, r.point_list()->y);
            unset_repetition();
        }
    }

    oc_out->set_layer_dt(tmp_ly, tmp_dt);
    return (ret);
}


bool
oas_cache::write_cached_wire(const rf_wire *w, int x, int y)
{
    bool ret = oc_out->setup_properties(w->boxpart.properties);
    if (ret) {
        Wire wo;
        wo.numpts = w->numpts + 1;
        wo.points = new Point[wo.numpts];
        wo.points[0].set(x, y);
        int i;
        for (i = 0; i < w->numpts; i++)
            wo.points[i+1].set(w->points[i].x + wo.points[0].x,
                w->points[i].y + wo.points[0].y);
        wo.set_wire_width(w->pw);
        wo.set_wire_style(w->style);
        oc_out->out_cache = 0;
        ret = oc_out->write_wire(&wo);
        oc_out->out_cache = this;
        delete [] wo.points;
    }
    oc_out->clear_property_queue();
    return (ret);
}


bool
oas_cache::write_wire_cache(const rf_wire *w)
{
    int tmp_ly, tmp_dt;
    oc_out->set_layer_dt(w->boxpart.layer, w->boxpart.datatype,
        &tmp_ly, &tmp_dt);

    bool ret = true;
    coor_t *rpts = w->boxpart.others;
    if (!rpts)
        ret = write_cached_wire(w, w->boxpart.x, w->boxpart.y);
    else {
        reps_t r(oc_minrun, oc_minarr, rpts);
        r.build();

        for (const twod_t *t = r.x_array_list(); t && ret; t = t->next) {
            set_repetition(t);
            ret = write_cached_wire(w, t->eltx, t->elty);
            unset_repetition();
        }
        for (const twod_t *t = r.y_array_list(); t && ret; t = t->next) {
            set_repetition(t);
            ret = write_cached_wire(w, t->eltx, t->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.x_run_list(); o && ret; o = o->next) {
            set_repetition(o);
            ret = write_cached_wire(w, o->eltx, o->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.y_run_list(); o && ret; o = o->next) {
            set_repetition(o);
            ret = write_cached_wire(w, o->eltx, o->elty);
            unset_repetition();
        }
        if (r.point_list() && ret) {
            set_repetition(r.point_list());
            ret = write_cached_wire(w, r.point_list()->x, r.point_list()->y);
            unset_repetition();
        }
    }

    oc_out->set_layer_dt(tmp_ly, tmp_dt);
    return (ret);
}


bool
oas_cache::write_cached_label(const rf_label *l, int x, int y)
{
    bool ret = oc_out->setup_properties(l->boxpart.properties);
    if (ret) {
        Text text;
        text.x = x;
        text.y = y;
        text.width = l->boxpart.width;
        text.height = l->boxpart.height;
        text.text = l->label;
        text.xform = l->xform;
        oc_out->out_cache = 0;
        ret = oc_out->write_text(&text);
        oc_out->out_cache = this;
    }
    oc_out->clear_property_queue();
    return (ret);
}


bool
oas_cache::write_label_cache(const rf_label *l)
{
    int tmp_ly, tmp_dt;
    oc_out->set_layer_dt(l->boxpart.layer, l->boxpart.datatype,
        &tmp_ly, &tmp_dt);

    bool ret = true;
    coor_t *rpts = l->boxpart.others;
    if (!rpts)
        ret = write_cached_label(l, l->boxpart.x, l->boxpart.y);
    else {
        reps_t r(oc_minrun, oc_minarr, rpts);
        r.build();

        for (const twod_t *t = r.x_array_list(); t && ret; t = t->next) {
            set_repetition(t);
            ret = write_cached_label(l, t->eltx, t->elty);
            unset_repetition();
        }
        for (const twod_t *t = r.y_array_list(); t && ret; t = t->next) {
            set_repetition(t);
            ret = write_cached_label(l, t->eltx, t->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.x_run_list(); o && ret; o = o->next) {
            set_repetition(o);
            ret = write_cached_label(l, o->eltx, o->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.y_run_list(); o && ret; o = o->next) {
            set_repetition(o);
            ret = write_cached_label(l, o->eltx, o->elty);
            unset_repetition();
        }
        if (r.point_list() && ret) {
            set_repetition(r.point_list());
            ret = write_cached_label(l, r.point_list()->x, r.point_list()->y);
            unset_repetition();
        }
    }

    oc_out->set_layer_dt(tmp_ly, tmp_dt);
    return (ret);
}


bool
oas_cache::write_cached_sref(const rf_sref *c, int x, int y)
{
    bool ret = oc_out->setup_properties(c->properties);
    if (ret) {
        Instance inst;
        inst.magn = c->tx.magn;
        inst.name = c->cname;
        inst.nx = 1;
        inst.ny = 1;
        inst.dx = 0;
        inst.dy = 0;
        inst.origin.set(x, y);
        inst.reflection = c->tx.refly;
        inst.set_angle(c->tx.ax, c->tx.ay);

        oc_out->out_cache = 0;
        ret = oc_out->write_sref(&inst);
        oc_out->out_cache = this;
    }
    oc_out->clear_property_queue();
    return (ret);
}


bool
oas_cache::write_sref_cache(const rf_sref *c)
{
    bool ret = true;
    coor_t *rpts = c->others;
    if (!rpts)
        ret = write_cached_sref(c, c->tx.tx, c->tx.ty);
    else {
        reps_t r(oc_minrun, oc_minarr, rpts);
        r.build();

        for (const twod_t *t = r.x_array_list(); t; t = t->next) {
            set_repetition(t);
            ret = write_cached_sref(c, t->eltx, t->elty);
            unset_repetition();
        }
        for (const twod_t *t = r.y_array_list(); t; t = t->next) {
            set_repetition(t);
            ret = write_cached_sref(c, t->eltx, t->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.x_run_list(); o; o = o->next) {
            set_repetition(o);
            ret = write_cached_sref(c, o->eltx, o->elty);
            unset_repetition();
        }
        for (const oned_t *o = r.y_run_list(); o; o = o->next) {
            set_repetition(o);
            ret = write_cached_sref(c, o->eltx, o->elty);
            unset_repetition();
        }
        if (r.point_list()) {
            set_repetition(r.point_list());
            ret = write_cached_sref(c, r.point_list()->x, r.point_list()->y);
            unset_repetition();
        }
    }
    return (ret);
}


void
oas_cache::set_repetition(const coor_t *p0)
{
    int cnt = 0;
    for (const coor_t *p = p0; p; p = p->next)
        cnt++;
    if (cnt < 2)
        return;

    oas_repetition &rep = oc_out->out_repetition;

    int sz = 2*(cnt-1);
    rep.type = 10;
    rep.xdim = cnt - 2;
    rep.array = new int[sz];
    coor_t *pn;
    int *a = rep.array;
    for (const coor_t *p = p0; p; p = pn) {
        pn = p->next;
        if (!pn)
            break;
        a[0] = pn->x - p->x;
        a[1] = pn->y - p->y;
        a += 2;
    }

    if (oc_gcd_check) {
        //  See if we can use a grid, which saves space.
        a = rep.array;

        int g2 = 1;
        bool dv2 = true;
        for (;;) {
            for (int i = 0; i < sz; i++) {
                if (a[i] & 1) {
                    dv2 = false;
                    break;
                }
            }
            if (!dv2)
                break;
            g2 <<= 1;
            for (int i = 0; i < sz; i++)
                a[i] >>= 1;
        }

        int g5 = 1;
        bool dv5 = true;
        for (;;) {
            for (int i = 0; i < sz; i++) {
                if (a[i] % 5) {
                    dv5 = false;
                    break;
                }
            }
            if (!dv5)
                break;
            g5 *= 5;
            for (int i = 0; i < sz; i++)
                a[i] /= 5;
        }

        unsigned int grid = 0;
        for (int i = 0; i < sz; i++) {
            grid = mmGCD(grid, abs(a[i]));
            if (grid == 1)
                break;
        }
        if (grid > 1) {
            for (int i = 0; i < sz; i++)
                a[i] /= (int)grid;
        }
        grid *= g2*g5;
        if (grid > 1) {
            rep.ydim = grid;
            rep.type = 11;
        }
    }
    oc_out->out_use_repetition = true;
}


void
oas_cache::set_repetition(const oned_t *od)
{
    oas_repetition &rep = oc_out->out_repetition;
    if (!od->dy) {
        // od->dx > 0 due to sort
        rep.type = 2;
        rep.xdim = od->nelts - 2;
        rep.xspace = od->dx;
    }
    else if (!od->dx) {
        // od->dy > 0 due to sort
        rep.type = 3;
        rep.ydim = od->nelts - 2;
        rep.yspace = od->dy;
    }
    else {
        rep.type = 9;
        rep.xdim = od->nelts - 2;
        rep.xspace = od->dx;
        rep.yspace = od->dy;
    }
    oc_out->out_use_repetition = true;
}


void
oas_cache::set_repetition(const twod_t *td)
{
    oas_repetition &rep = oc_out->out_repetition;
    if (!td->dy1 && !td->dx2) {
        rep.type = 1;
        rep.xdim = td->nelts1 - 2;
        rep.ydim = td->nelts2 - 2;
        rep.xspace = td->dx1;
        rep.yspace = td->dy2;
    }
    else if (!td->dx1 && !td->dy2) {
        rep.type = 1;
        rep.xdim = td->nelts2 - 2;
        rep.ydim = td->nelts1 - 2;
        rep.xspace = td->dx2;
        rep.yspace = td->dy1;
    }
    else {
        rep.type = 8;
        rep.xdim = td->nelts1 - 2;
        rep.ydim = td->nelts2 - 2;
        int *array = new int[4];
        rep.array = array;
        array[0] = td->dx1;
        array[1] = td->dy1;
        array[2] = td->dx2;
        array[3] = td->dy2;
    }
    oc_out->out_use_repetition = true;
}


void
oas_cache::unset_repetition()
{
    oc_out->out_repetition.reset(-1);
    oc_out->out_use_repetition = false;
}
// End of oas_cache functions.


//----------------------------------------------------------------------------
// Elements use in cache

namespace {
    // Return true if the property lists differ.  Note that we don't bother
    // to check ordering.
    //
    bool
    pdiff(CDp *p1, CDp *p2)
    {
        if (p1 == p2)
            return (false);
        while (p1 && p2) {
            if (p1->value() != p2->value())
                return (true);
            if (strcmp(p1->string() ? p1->string() : "",
                    p2->string() ? p2->string() : ""))
                return (true);
            p1 = p1->next_prp();
            p2 = p2->next_prp();
        }
        if (p1 || p2)
            return (true);
        return (false);
    }
}


//------- Box Element --------------------------------------------------------
// rf_box is a cache element for a rectangle.

bool
rf_box::operator==(const rf_box &b) const
{
    if (layer != b.layer)
        return (false);
    if (datatype != b.datatype)
        return (false);
    if (width != b.width)
        return (false);
    if (height != b.height)
        return (false);
    if (pdiff(properties, b.properties))
        return (false);
    return (true);
}


bool
rf_box::operator<(const rf_box &b) const
{
    if (layer < b.layer)
        return (true);
    if (layer == b.layer) {
        if (width < b.width)
            return (true);
    }
    return (false);
}


unsigned int
rf_box::hash()
{
    unsigned int k = INCR_HASH_INIT;
    k = incr_hash(k, &layer);
    k = incr_hash(k, &datatype);
    k = incr_hash(k, &width);
    k = incr_hash(k, &height);
    for (CDp *p = properties; p; p = p->next_prp()) {
        int val = p->value();
        k = incr_hash(k, &val);
        k = incr_hash_string(k, p->string());
    }
    return (k);
}


// Static function.
// Support for optimizing write order for modality.
//
unsigned int
rf_box::diff(const rf_box *a, const rf_box *b)
{
    unsigned int d = 0;
    if (!b) {
        if (a->layer != 0)
            d += 2;
        if (a->datatype != 0)
            d += 2;
        if (a->width != 0)
            d += 4;
        if (a->height != 0)
            d += 4;
        if (a->x != 0)
            d += 4;
        if (a->y != 0)
            d += 4;
    }
    else {
        if (a->layer != b->layer)
            d += 2;
        if (a->datatype != b->datatype)
            d += 2;
        if (a->width != b->width)
            d += 4;
        if (a->height != b->height)
            d += 4;
        if (a->x != b->x)
            d += 4;
        if (a->y != b->y)
            d += 4;
    }
    return (d);
}


//------- Poly Element -------------------------------------------------------
// rf_poly is a cache element for a polygon.

bool
rf_poly::operator==(const rf_poly &p) const
{
    if (!(boxpart == p.boxpart))
        return (false);
    if (numpts != p.numpts)
        return (false);
    for (int i = 0; i < numpts; i++) {
        if (points[i] != p.points[i])
            return (false);
    }
    return (true);
}


bool
rf_poly::operator<(const rf_poly &p) const
{
    if (boxpart.layer < p.boxpart.layer)
        return (true);
    return (false);
}


unsigned int
rf_poly::hash()
{
    unsigned int k = boxpart.hash();
    for (int i = 0; i < numpts; i++) {
        k = incr_hash(k, &points[i].x);
        k = incr_hash(k, &points[i].y);
    }
    return (k);
}


// Static function.
// Support for optimizing write order for modality.
//
unsigned int
rf_poly::diff(const rf_poly *a, const rf_poly *b)
{
    unsigned int d = rf_box::diff(&a->boxpart, &b->boxpart);
    if (!b) {
        if (a->numpts != 0)
            return (d + 2 + 20);
    }
    else {
        if (a->numpts != b->numpts)
            return (d + 2 + 20);
        int dx = a->points[0].x - b->points[0].x;
        int dy = a->points[0].y - b->points[0].y;
        for (int i = 1; i < a->numpts; i++) {
            if (a->points[i].x - b->points[i].x != dx ||
                    a->points[i].y - b->points[i].y != dy)
                return (d + 20);
        }
    }
    return (d);
}


//------- Wire Element -------------------------------------------------------
// rf_wire is a cache element for a path.

bool
rf_wire::operator==(const rf_wire &w) const
{
    if (!(boxpart == w.boxpart))
        return (false);
    if (numpts != w.numpts)
        return (false);
    for (int i = 0; i < numpts; i++) {
        if (points[i] != w.points[i])
            return (false);
    }
    if (pw != w.pw)
        return (false);
    if (style != w.style)
        return (false);
    return (true);
}


bool
rf_wire::operator<(const rf_wire &w) const
{
    if (boxpart.layer < w.boxpart.layer)
        return (true);
    return (false);
}


unsigned int
rf_wire::hash()
{
    unsigned int k = boxpart.hash();
    for (int i = 0; i < numpts; i++) {
        k = incr_hash(k, &points[i].x);
        k = incr_hash(k, &points[i].y);
    }
    k = incr_hash(k, &pw);
    k = incr_hash(k, &style);
    return (k);
}


// Static function.
// Support for optimizing write order for modality.
//
unsigned int
rf_wire::diff(const rf_wire *a, const rf_wire *b)
{
    unsigned int d = rf_box::diff(&a->boxpart, &b->boxpart);
    if (!b) {
        if (a->pw != 0)
            d += 2;
        if (a->style != 0)
            d += 2;
        if (a->numpts != 0)
            return (d + 2 + 20);
    }
    else {
        if (a->pw != b->pw)
            d += 2;
        if (a->style != b->style)
            d += 2;
        if (a->numpts != b->numpts)
            return (d + 2 + 20);
        int dx = a->points[0].x - b->points[0].x;
        int dy = a->points[0].y - b->points[0].y;
        for (int i = 1; i < a->numpts; i++) {
            if (a->points[i].x - b->points[i].x != dx ||
                    a->points[i].y - b->points[i].y != dy)
                return (d + 20);
        }
    }
    return (d);
}


//------- Label Element ------------------------------------------------------
// rf_label is a cache element for a label.

bool
rf_label::operator==(const rf_label &l) const
{
    if (!(boxpart == l.boxpart))
        return (false);
    if (strcmp(label, l.label))
        return (false);
    if (xform != l.xform)
        return (false);
    return (true);
}


bool
rf_label::operator<(const rf_label &l) const
{
    if (boxpart.layer < l.boxpart.layer)
        return (true);
    return (false);
}


unsigned int
rf_label::hash()
{
    unsigned int k = boxpart.hash();
    k = incr_hash_string(k, label);
    k = incr_hash(k, &xform);
    return (k);
}


// Static function.
// Support for optimizing write order for modality.
//
unsigned int
rf_label::diff(const rf_label *a, const rf_label *b)
{
    unsigned int d = rf_box::diff(&a->boxpart, &b->boxpart);
    if (!b) {
        if (a->xform != 0)
            d += 2;
    }
    else {
        if (a->xform != b->xform)
            d += 2;
        if (strcmp(a->label, b->label))
            d += 4;
    }
    return (d);
}


//------- Sref Element -------------------------------------------------------
// rf_sref is a cache element for an unarrayed subcell.

bool
rf_sref::operator==(const rf_sref &c) const
{
    if (strcmp(cname, c.cname))
        return (false);
    if (fabs(tx.magn - c.tx.magn) > 1e-12)
        return (false);
    if (tx.ax != c.tx.ax || tx.ay != c.tx.ay)
        return (false);
    if (tx.refly != c.tx.refly)
        return (false);
    if (pdiff(properties, c.properties))
        return (false);
    return (true);
}


bool
rf_sref::operator<(const rf_sref &c) const
{
    if (strcmp(cname, c.cname) < 0)
        return (true);
    return (false);
}


unsigned int
rf_sref::hash()
{
    unsigned int k = INCR_HASH_INIT;
    k = incr_hash(k, &tx.ax);
    k = incr_hash(k, &tx.ay);
    k = incr_hash(k, &tx.refly);
    k = incr_hash(k, &tx.magn);
    k = incr_hash_string(k, cname);
    for (CDp *p = properties; p; p = p->next_prp()) {
        int val = p->value();
        k = incr_hash(k, &val);
        k = incr_hash_string(k, p->string());
    }
    return (k);
}


// Static function.
// Support for optimizing write order for modality.
//
unsigned int
rf_sref::diff(const rf_sref *a, const rf_sref *b)
{
    unsigned int d = 0;
    if (!b) {
        if (a->tx.tx != 0)
            d += 4;
        if (a->tx.ty != 0)
            d += 4;
        if (a->tx.ax != 1 || a->tx.ay != 0 || a->tx.refly)
            d += 1;
        if (a->tx.magn != 1.0)
            d += 8;
    }
    else {
        if (a->tx.tx != b->tx.tx)
            d += 4;
        if (a->tx.ty != b->tx.ty)
            d += 4;
        if (a->tx.ax != b->tx.ax || a->tx.ay != b->tx.ay ||
                a->tx.refly != b->tx.refly)
            d += 1;
        if (a->tx.magn != b->tx.magn)
            d += 8;
        if (strcmp(a->cname, b->cname))
            d += 4;
    }
    return (d);
}


//----------------------------------------------------------------------------
// The code below implements an algorithm to recognize periodic groups
// of points.  The points are the locations of identical objects, and the
// groupings are converted to a repetition record.


//------- reps_t functions ---------------------------------------------------
// reps_t provides containers for the runs and arrays, and performs
// the grouping.


// Do the work.
//
void
reps_t::build()
{
    x_lists = get_row_runs();
    y_lists = get_col_runs();

    if (arraymin > 0) {
        get_row_arrays();
        get_col_arrays();
    }

    // This seems to shrink output for some reason.
    sort_residual_by_col();

#ifdef REP_DEBUG
    for (twod_t *t = x_arrays; t; t = t->next)
        printf(
            "  array x=%d y=%d n1=%d dx1=%d dy1=%d n2=%d dx2=%d dy2=%d\n",
            t->eltx, t->elty, t->nelts1, t->dx1, t->dy1,
            t->nelts2, t->dx2, t->dy2);
    for (twod_t *t = y_arrays; t; t = t->next)
        printf(
            "  array x=%d y=%d n1=%d dx1=%d dy1=%d n2=%d dx2=%d dy2=%d\n",
            t->eltx, t->elty, t->nelts1, t->dx1, t->dy1,
            t->nelts2, t->dx2, t->dy2);
    for (oned_t *o = x_lists; o; o = o->next)
        printf("  list x=%d y=%d n=%d dx=%d dy=%d\n",  o->eltx, o->elty,
            o->nelts, o->dx, o->dy);
    for (oned_t *o = y_lists; o; o = o->next)
        printf("  list x=%d y=%d n=%d dx=%d dy=%d\n",  o->eltx, o->elty,
            o->nelts, o->dx, o->dy);
    if (residual) {
        int cnt = 0;
        for (coor_t *p = residual; p; p = p->next)
            cnt++;
        printf("  random placement: %d\n", cnt);
    }
#endif
}


oned_t *
reps_t::get_row_runs()
{
    // Warning: runmin must be 4 or larger, or seg. faults result.

    if (!residual)
        return (0);
    sort_residual_by_row();

    // Remove duplicates, these are coincident objects/subcells.
    {
        coor_t *pp = residual;
        for (coor_t *p = pp->next; p; p = p->next) {
            if (p->x == pp->x && p->y == pp->y) {
                pp->next = p->next;
                continue;
            }
            pp = p;
        }
    }

    coor_t *pts = residual;
    residual = 0;

    oned_t *x_runs = 0, *oe = 0;
    while (pts) {

        // Snip the part with equal y.
        int cnt = 1;
        coor_t *pn;
        {
            coor_t *ptmp = pts;
            while (ptmp->next && ptmp->next->y == pts->y) {
                cnt++;
                ptmp = ptmp->next;
            }
            pn = ptmp->next;
            ptmp->next = 0;
        }

        while (pts) {
            if (cnt < runmin) {
                // Not enough for a run, put in residual.
                while (pts) {
                    coor_t *p = pts;
                    pts = pts->next;
                    p->next = residual;
                    residual = p;
                }
                break;
            }

            int n1 = 2;
            int n2 = 2;
            int n3 = 2;

            coor_t *ptmp = pts->next;
            int dx1 = ptmp->x - pts->x;
            int l1 = ptmp->x + dx1;
            ptmp = ptmp->next;
            if (ptmp->x == l1) {
                n1++;
                l1 += dx1;
            }
            int dx2 = ptmp->x - pts->x;
            int l2 = ptmp->x + dx2;
            ptmp = ptmp->next;
            if (ptmp->x == l1) {
                n1++;
                l1 += dx1;
            }
            if (ptmp->x == l2) {
                n2++;
                l2 += dx2;
            }
            int dx3 = ptmp->x - pts->x;
            int l3 = ptmp->x + dx3;
            for (ptmp = ptmp->next; ptmp; ptmp = ptmp->next) {
                if (ptmp->x == l1) {
                    n1++;
                    l1 += dx1;
                }
                if (ptmp->x == l2) {
                    n2++;
                    l2 += dx2;
                }
                if (ptmp->x == l3) {
                    n3++;
                    l3 += dx3;
                }
                if (ptmp->x > l1 && ptmp->x > l2 && ptmp->x > l3)
                    break;
            }

            int n = n1;
            int d = dx1;
            if (n < n2) {
                n = n2;
                d = dx2;
            }
            if (n < n3) {
                n = n3;
                d = dx3;
            }
            if (n < runmin) {
                // No run here.
                coor_t *p = pts;
                pts = pts->next;
                p->next = residual;
                residual = p;
                cnt--;
                continue;
            }

            // Create run list element.
            oned_t *o = new_oned(pts->x, pts->y, d, 0, n);
            if (x_runs) {
                oe->next = o;
                oe = o;
            }
            else
                oe = x_runs = o;

            // Remove run points and put them in unused list.
            int last = pts->x + d;
            coor_t *pt = pts;
            pts = pts->next;
            pt->next = unused;
            unused = pt;
            cnt -= n;

            coor_t *pv = 0, *px;
            for (coor_t *p = pts; p; p = px) {
                px = p->next;
                if (p->x == last) {
                    last += d;
                    if (pv)
                        pv->next = px;
                    else
                        pts = px;
                    p->next = unused;
                    unused = p;
                    continue;
                }
                if (p->x > last)
                    break;
                pv = p;
            }
        }
        pts = pn;
    }
    return (x_runs);
}


oned_t *
reps_t::get_col_runs()
{
    // Warning: runmin must be 4 or larger, or seg. faults result.

    if (!residual)
        return (0);
    sort_residual_by_col();

    coor_t *pts = residual;
    residual = 0;

    oned_t *y_runs = 0, *oe = 0;
    while (pts) {

        // Snip the part with equal x.
        int cnt = 1;
        coor_t *pn;
        {
            coor_t *ptmp = pts;
            while (ptmp->next && ptmp->next->x == pts->x) {
                cnt++;
                ptmp = ptmp->next;
            }
            pn = ptmp->next;
            ptmp->next = 0;
        }

        while (pts) {
            if (cnt < runmin) {
                // Not enough for a run, put in residual.
                while (pts) {
                    coor_t *p = pts;
                    pts = pts->next;
                    p->next = residual;
                    residual = p;
                }
                break;
            }

            int n1 = 2;
            int n2 = 2;
            int n3 = 2;

            coor_t *ptmp = pts->next;
            int dy1 = ptmp->y - pts->y;
            int l1 = ptmp->y + dy1;
            ptmp = ptmp->next;
            if (ptmp->y == l1) {
                n1++;
                l1 += dy1;
            }
            int dy2 = ptmp->y - pts->y;
            int l2 = ptmp->y + dy2;
            ptmp = ptmp->next;
            if (ptmp->y == l1) {
                n1++;
                l1 += dy1;
            }
            if (ptmp->y == l2) {
                n2++;
                l2 += dy2;
            }
            int dy3 = ptmp->y - pts->y;
            int l3 = ptmp->y + dy3;
            for (ptmp = ptmp->next; ptmp; ptmp = ptmp->next) {
                if (ptmp->y == l1) {
                    n1++;
                    l1 += dy1;
                }
                if (ptmp->y == l2) {
                    n2++;
                    l2 += dy2;
                }
                if (ptmp->y == l3) {
                    n3++;
                    l3 += dy3;
                }
                if (ptmp->y > l1 && ptmp->y > l2 && ptmp->y > l3)
                    break;
            }

            int n = n1;
            int d = dy1;
            if (n < n2) {
                n = n2;
                d = dy2;
            }
            if (n < n3) {
                n = n3;
                d = dy3;
            }
            if (n < runmin) {
                // No run here.
                coor_t *p = pts;
                pts = pts->next;
                p->next = residual;
                residual = p;
                cnt--;
                continue;
            }

            // Create run list element.
            oned_t *o = new_oned(pts->x, pts->y, 0, d, n);
            if (y_runs) {
                oe->next = o;
                oe = o;
            }
            else
                oe = y_runs = o;

            // Remove run points and put them in unused list.
            int last = pts->y + d;
            coor_t *pt = pts;
            pts = pts->next;
            pt->next = unused;
            unused = pt;
            cnt -= n;

            coor_t *pv = 0, *px;
            for (coor_t *p = pts; p; p = px) {
                px = p->next;
                if (p->y == last) {
                    last += d;
                    if (pv)
                        pv->next = px;
                    else
                        pts = px;
                    p->next = unused;
                    unused = p;
                    continue;
                }
                if (p->y > last)
                    break;
                pv = p;
            }
        }
        pts = pn;
    }
    return (y_runs);
}


namespace {
    // Comparison functions, sort ascending in y, x.

    inline bool
    row_cmp(const coor_t *p1, const coor_t *p2)
    {
        if (p1->y < p2->y)
            return (true);
        if (p1->y > p2->y)
            return (false);
        return (p1->x < p2->x);
    }

    inline bool
    col_cmp(const coor_t *p1, const coor_t *p2)
    {
        if (p1->x < p2->x)
            return (true);
        if (p1->x > p2->x)
            return (false);
        return (p1->y < p2->y);
    }
}


void
reps_t::sort_residual_by_row()
{
    coor_t *p0 = residual;
    if (p0 && p0->next) {
        int len = 0;
        for (coor_t *p = p0; p; p = p->next)
            len++;
        if (len == 2) {
            if (row_cmp(p0, p0->next))
                return;
            residual = p0->next;
            residual->next = p0;
            p0->next = 0;
            return;
        }
        coor_t **ary = new coor_t*[len];
        int i = 0;
        for (coor_t *p = p0; p; p = p->next, i++)
            ary[i] = p;
        std::sort(ary, ary + len, row_cmp);
        len--;
        for (i = 0; i < len; i++)
            ary[i]->next = ary[i+1];
        ary[i]->next = 0;
        residual = ary[0];
        delete [] ary;
    }
}


void
reps_t::sort_residual_by_col()
{
    coor_t *p0 = residual;
    if (p0 && p0->next) {
        int len = 0;
        for (coor_t *p = p0; p; p = p->next)
            len++;
        if (len == 2) {
            if (col_cmp(p0, p0->next))
                return;
            residual = p0->next;
            residual->next = p0;
            p0->next = 0;
            return;
        }
        coor_t **ary = new coor_t*[len];
        int i = 0;
        for (coor_t *p = p0; p; p = p->next, i++)
            ary[i] = p;
        std::sort(ary, ary + len, col_cmp);
        len--;
        for (i = 0; i < len; i++)
            ary[i]->next = ary[i+1];
        ary[i]->next = 0;
        residual = ary[0];
        delete [] ary;
    }
}


namespace {
    inline bool
    r_compat(oned_t *o1, oned_t *o2)
    {
        if (o1->dx != o2->dx)
            return (false);
        if ((o2->eltx - o1->eltx) % o1->dx)
            return (false);
        return (true);
    }

    inline bool
    c_compat(oned_t *o1, oned_t *o2)
    {
        if (o1->dy != o2->dy)
            return (false);
        if ((o2->elty - o1->elty) % o1->dy)
            return (false);
        return (true);
    }
}


// Return a list of all runs compatible with the first run in listp,
// meaning that they all have the same spacing and are on the same
// grid.
//
oned_t *
reps_t::row_compat_list(oned_t **listp)
{
    oned_t *od = *listp;

    oned_t *o0 = od;
    oned_t *oe = o0;
    od = od->next;
    oe->next = 0;

    oned_t *op = 0, *on;
    for (oned_t *o = od; o; o = on) {
        on = o->next;
        if (r_compat(o0, o)) {
            if (op)
                op->next = on;
            else
                od = on;
            o->next = 0;
            oe->next = o;
            oe = oe->next;
            continue;
        }
        op = o;
    }
    *listp = od;
    return (o0);
}


oned_t *
reps_t::col_compat_list(oned_t **listp)
{
    oned_t *od = *listp;

    oned_t *o0 = od;
    oned_t *oe = o0;
    od = od->next;
    oe->next = 0;

    oned_t *op = 0, *on;
    for (oned_t *o = od; o; o = on) {
        on = o->next;
        if (c_compat(o0, o)) {
            if (op)
                op->next = on;
            else
                od = on;
            o->next = 0;
            oe->next = o;
            oe = oe->next;
            continue;
        }
        op = o;
    }
    *listp = od;
    return (o0);
}


void
reps_t::get_row_arrays()
{
    oned_t *od = x_lists;
    x_lists = 0;

    while (od) {
        oned_t *cplist = row_compat_list(&od);

        int cnt = 1;
        {
            oned_t *op = cplist;
            for (oned_t *o = cplist->next; o; o = o->next) {
                if (o->elty != op->elty)
                    cnt++;
                op = o;
            }
        }
        if (cnt < arraymin) {
            while (cplist) {
                oned_t *c = cplist;
                cplist = cplist->next;
                c->next = x_lists;
                x_lists = c;
            }
            continue;
        }

        oned_t **ary = new oned_t*[cnt];
        {
            cnt = 1;
            ary[0] = cplist;
            oned_t *op = cplist, *on;
            for (oned_t *o = cplist->next; o; o = on) {
                on = o->next;
                if (o->elty != op->elty) {
                    op->next = 0;
                    ary[cnt++] = o;
                }
                op = o;
            }
        }
        int ntop = cnt - arraymin + 1;
        for (int i = 0; i < ntop; ) {
            if (!ary[i]) {
                i++;
                continue;
            }
            oned_t *oret = find_row_array(ary + i, cnt - i);
            if (oret) {
                // convert to twod_t
                twod_t *t = new_twod(oret);
                t->next = x_arrays;
                x_arrays = t;
            }
            else {
                oned_t *o = ary[i];
                ary[i] = ary[i]->next;
                o->next = x_lists;
                x_lists = o;
            }
            if (!ary[i])
                i++;
        }
        for (int i = ntop; i < cnt; i++) {
            oned_t *ox = ary[i];
            while (ox) {
                oned_t *o = ox;
                ox = ox->next;
                o->next = x_lists;
                x_lists = o;
            }
        }
        delete [] ary;
    }
}


void
reps_t::get_col_arrays()
{
    oned_t *od = y_lists;
    y_lists = 0;

    while (od) {
        oned_t *cplist = col_compat_list(&od);

        int cnt = 1;
        {
            oned_t *op = cplist;
            for (oned_t *o = cplist->next; o; o = o->next) {
                if (o->eltx != op->eltx)
                    cnt++;
                op = o;
            }
        }
        if (cnt < arraymin) {
            while (cplist) {
                oned_t *c = cplist;
                cplist = cplist->next;
                c->next = y_lists;
                y_lists = c;
            }
            continue;
        }

        oned_t **ary = new oned_t*[cnt];
        {
            cnt = 1;
            ary[0] = cplist;
            oned_t *op = cplist, *on;
            for (oned_t *o = cplist->next; o; o = on) {
                on = o->next;
                if (o->eltx != op->eltx) {
                    op->next = 0;
                    ary[cnt++] = o;
                }
                op = o;
            }
        }
        int ntop = cnt - arraymin + 1;
        for (int i = 0; i < ntop; ) {
            if (!ary[i]) {
                i++;
                continue;
            }
            oned_t *oret = find_col_array(ary + i, cnt - i);
            if (oret) {
                // convert to twod_t
                twod_t *t = new_twod(oret);
                t->next = y_arrays;
                y_arrays = t;
            }
            else {
                oned_t *o = ary[i];
                ary[i] = ary[i]->next;
                o->next = y_lists;
                y_lists = o;
            }
            if (!ary[i])
                i++;
        }
        for (int i = ntop; i < cnt; i++) {
            oned_t *ox = ary[i];
            while (ox) {
                oned_t *o = ox;
                ox = ox->next;
                o->next = y_lists;
                y_lists = o;
            }
        }
        delete [] ary;
    }
}


// Return the oned_t that has matching row values found above *ip
// in the array.  If found, *ip is updated to the new row.
//
inline oned_t *
reps_t::next_row_poss(int xl, int xr, int *ip, oned_t **ary, int asize)
{
    int i = *ip + 1;
    for ( ; i < asize; i++) {
        for (oned_t *o = ary[i]; o; o = o->next) {
            int xl1 = o->eltx;
            if (xl1 >= xr)
                break;
            int xr1 = xl1 + (o->nelts - 1)*o->dx;
            int mn = mmMax(xl, xl1);
            int mx = mmMin(xr, xr1);
            if ((mx - mn)/o->dx + 1 >= runmin) {
                *ip = i;
                return (o);
            }
        }
    }
    return (0);
}


inline bool
reps_t::row_upd(int &xln, int &xrn, int xl, int xr, int dx)
{
    int mn = mmMax(xl, xln);
    int mx = mmMin(xr, xrn);
    if ((mx - mn)/dx + 1 >= runmin) {
        xln = mn;
        xrn = mx;
        return (true);
    }
    return (false);
}


// Return a run list starting from the first element of ary, that
// forms an array.  If a run list is returned, the elements have
// been removed from ary.
//
oned_t *
reps_t::find_row_array(oned_t **ary, int asize)
{
    oned_t *od = ary[0];

    int dx = od->dx;
    int odxl = od->eltx;
    int odxr = odxl + (od->nelts - 1)*dx;
    int xl1 = odxl;
    int xl2 = xl1;
    int xl3 = xl1;
    int xr1 = odxr;
    int xr2 = xr1;
    int xr3 = xr1;

    int j = 0;
    oned_t *otmp = next_row_poss(odxl, odxr, &j, ary, asize);
    if (!otmp)
        return (0);

    int dy1 = 0;
    int dy2 = 0;
    int dy3 = 0;
    int l1 = od->elty;
    int l2 = l1;
    int l3 = l1;
    int n1 = 1;
    int n2 = 1;
    int n3 = 1;

    int xl = otmp->eltx;
    int xr = xl + (otmp->nelts - 1)*dx;
    if (row_upd(xl1, xr1, xl, xr, dx)) {
        dy1 = otmp->elty - od->elty;
        l1 += 2*dy1;
        n1++;
    }

    otmp = next_row_poss(odxl, odxr, &j, ary, asize);
    if (otmp) {
        xl = otmp->eltx;
        xr = xl + (otmp->nelts - 1)*dx;
        if (otmp->elty == l1 && row_upd(xl1, xr1, xl, xr, dx)) {
            l1 += dy1;
            n1++;
        }
        if (row_upd(xl2, xr2, xl, xr, dx)) {
            dy2 = otmp->elty - od->elty;
            l2 += 2*dy2;
            n2++;
        }
        otmp = next_row_poss(odxl, odxr, &j, ary, asize);
        if (otmp) {
            xl = otmp->eltx;
            xr = xl + (otmp->nelts - 1)*dx;
            if (otmp->elty == l1 && row_upd(xl1, xr1, xl, xr, dx)) {
                l1 += dy1;
                n1++;
            }
            if (otmp->elty == l2 && row_upd(xl2, xr2, xl, xr, dx)) {
                l2 += dy2;
                n2++;
            }
            if (row_upd(xl3, xr3, xl, xr, dx)) {
                dy3 = otmp->elty - od->elty;
                l3 += 2*dy3;
                n3++;
            }
            otmp = next_row_poss(odxl, odxr, &j, ary, asize);
            while (otmp) {
                xl = otmp->eltx;
                xr = xl + (otmp->nelts - 1)*dx;
                if (otmp->elty == l1 && row_upd(xl1, xr1, xl, xr, dx)) {
                    l1 += dy1;
                    n1++;
                }
                if (otmp->elty == l2 && row_upd(xl2, xr2, xl, xr, dx)) {
                    l2 += dy2;
                    n2++;
                }
                if (otmp->elty == l3 && row_upd(xl3, xr3, xl, xr, dx)) {
                    l3 += dy3;
                    n3++;
                }
                if (otmp->elty > l1 && otmp->elty > l2 &&
                        otmp->elty > l3)
                    break;
                otmp = next_row_poss(odxl, odxr, &j, ary, asize);
            }
        }
    }

    int minval = xl1;
    int maxval = xr1;
    int nn = n1 >= arraymin ? ((xr1 - xl1)/dx + 1)*n1 : 0;
    int d = dy1;
    int n = n1;
    if (n2 >= arraymin) {
        int nn2 = ((xr2 - xl2)/dx + 1)*n2;
        if (nn < nn2) {
            minval = xl2;
            maxval = xr2;
            nn = nn2;
            d = dy2;
            n = n2;
        }
    }
    if (n3 >= arraymin) {
        int nn3 = ((xr3 - xl3)/dx + 1)*n3;
        if (nn < nn3) {
            minval = xl3;
            maxval = xr3;
            nn = nn3;
            d = dy3;
            n = n3;
        }
    }

    if (n < arraymin)
        return (0);

    oned_t *o0 = 0, *oe = 0;
    int last = od->elty;
    for (int i = 0; i < asize; i++) {
        if (!ary[i])
            continue;
        if (ary[i]->elty == last) {
            last += d;
            oned_t *op = 0, *on;
            for (oned_t *o = ary[i]; o; o = on) {
                on = o->next;
                xl = o->eltx;
                xr = xl + (o->nelts-1)*dx;

                if (xl <= minval && xr >= maxval) {
                    if (op)
                        op->next = on;
                    else
                        ary[i] = on;
                    o->next = 0;
                    if (xl < minval) {
                        int ne = (minval - xl)/dx;
                        if (ne >= runmin) {
                            oned_t *onew = new_oned(xl, o->elty, o->dx, 0,
                                ne);
                            if (op) {
                                onew->next = op->next;
                                op->next = onew;
                            }
                            else {
                                onew->next = ary[i];
                                ary[i] = onew;
                            }
                            op = onew;
                        }
                        else {
                            for (int x = xl; x < minval; x += dx) {
                                coor_t *p = unused;
                                unused = unused->next;
                                p->x = x;
                                p->y = o->elty;
                                p->next = residual;
                                residual = p;
                            }
                        }
                    }
                    if (maxval < xr) {
                        int ne = (xr - maxval)/dx;
                        if (ne >= runmin) {
                            oned_t *onew = new_oned(maxval + dx, o->elty,
                                o->dx, 0, ne);
                            if (op) {
                                onew->next = op->next;
                                op->next = onew;
                            }
                            else {
                                onew->next = ary[i];
                                ary[i] = onew;
                            }
                            op = onew;
                        }
                        else {
                            for (int x = maxval + dx; x <= xr; x += dx) {
                                coor_t *p = unused;
                                unused = unused->next;
                                p->x = x;
                                p->y = o->elty;
                                p->next = residual;
                                residual = p;
                            }
                        }
                    }
                    o->eltx = minval;
                    o->nelts = (maxval - minval)/dx + 1;
                    if (o0) {
                        oe->next = o;
                        oe = o;
                    }
                    else
                        o0 = oe = o;
                    n--;
                    break;
                }
                if (xl >= maxval)
                    break;
                op = o;
            }
            if (!n)
                break;
        }
    }

    return (o0);
}


inline oned_t *
reps_t::next_col_poss(int yb, int yt, int *ip, oned_t **ary, int asize)
{
    int i = *ip + 1;
    for ( ; i < asize; i++) {
        for (oned_t *o = ary[i]; o; o = o->next) {
            int yb1 = o->elty;
            if (yb1 >= yt)
                break;
            int yt1 = yb1 + (o->nelts - 1)*o->dy;
            int mn = mmMax(yb, yb1);
            int mx = mmMin(yt, yt1);
            if ((mx - mn)/o->dy + 1 >= runmin) {
                *ip = i;
                return (o);
            }
        }
    }
    return (0);
}


inline bool
reps_t::col_upd(int &ybn, int &ytn, int yb, int yt, int dy)
{
    int mn = mmMax(yb, ybn);
    int mx = mmMin(yt, ytn);
    if ((mx - mn)/dy + 1 >= runmin) {
        ybn = mn;
        ytn = mx;
        return (true);
    }
    return (false);
}


// Return a run list starting from the first element of ary, that
// forms an array.  If a run list is returned, the elements have
// been removed from ary.
//
oned_t *
reps_t::find_col_array(oned_t **ary, int asize)
{
    oned_t *od = ary[0];

    int dy = od->dy;
    int odyb = od->elty;
    int odyt = odyb + (od->nelts - 1)*dy;
    int yb1 = odyb;
    int yb2 = yb1;
    int yb3 = yb1;
    int yt1 = odyt;
    int yt2 = yt1;
    int yt3 = yt1;

    int j = 0;
    oned_t *otmp = next_col_poss(odyb, odyt, &j, ary, asize);
    if (!otmp)
        return (0);

    int dx1 = 0;
    int dx2 = 0;
    int dx3 = 0;
    int l1 = od->eltx;
    int l2 = l1;
    int l3 = l1;
    int n1 = 1;
    int n2 = 1;
    int n3 = 1;

    int yb = otmp->elty;
    int yt = yb + (otmp->nelts - 1)*dy;
    if (col_upd(yb1, yt1, yb, yt, dy)) {
        dx1 = otmp->eltx - od->eltx;
        l1 += 2*dx1;
        n1++;
    }

    otmp = next_col_poss(odyb, odyt, &j, ary, asize);
    if (otmp) {
        yb = otmp->elty;
        yt = yb + (otmp->nelts - 1)*dy;
        if (otmp->eltx == l1 && col_upd(yb1, yt1, yb, yt, dy)) {
            l1 += dx1;
            n1++;
        }
        if (col_upd(yb2, yt2, yb, yt, dy)) {
            dx2 = otmp->eltx - od->eltx;
            l2 += 2*dx2;
            n2++;
        }
        otmp = next_col_poss(odyb, odyt, &j, ary, asize);
        if (otmp) {
            yb = otmp->elty;
            yt = yb + (otmp->nelts - 1)*dy;
            if (otmp->eltx == l1 && col_upd(yb1, yt1, yb, yt, dy)) {
                l1 += dx1;
                n1++;
            }
            if (otmp->eltx == l2 && col_upd(yb2, yt2, yb, yt, dy)) {
                l2 += dx2;
                n2++;
            }
            if (col_upd(yb3, yt3, yb, yt, dy)) {
                dx3 = otmp->eltx - od->eltx;
                l3 += 2*dx3;
                n3++;
            }
            otmp = next_col_poss(odyb, odyt, &j, ary, asize);
            while (otmp) {
                yb = otmp->elty;
                yt = yb + (otmp->nelts - 1)*dy;
                if (otmp->eltx == l1 && col_upd(yb1, yt1, yb, yt, dy)) {
                    l1 += dx1;
                    n1++;
                }
                if (otmp->eltx == l2 && col_upd(yb2, yt2, yb, yt, dy)) {
                    l2 += dx2;
                    n2++;
                }
                if (otmp->eltx == l3 && col_upd(yb3, yt3, yb, yt, dy)) {
                    l3 += dx3;
                    n3++;
                }
                if (otmp->eltx > l1 && otmp->eltx > l2 &&
                        otmp->eltx > l3)
                    break;
                otmp = next_col_poss(odyb, odyt, &j, ary, asize);
            }
        }
    }

    int minval = yb1;
    int maxval = yt1;
    int nn = n1 >= arraymin ? ((yt1 - yb1)/dy + 1)*n1 : 0;
    int d = dx1;
    int n = n1;
    if (n2 >= arraymin) {
        int nn2 = ((yt2 - yb2)/dy + 1)*n2;
        if (nn < nn2) {
            minval = yb2;
            maxval = yt2;
            nn = nn2;
            d = dx2;
            n = n2;
        }
    }
    if (n3 >= arraymin) {
        int nn3 = ((yt3 - yb3)/dy + 1)*n3;
        if (nn < nn3) {
            minval = yb3;
            maxval = yt3;
            nn = nn3;
            d = dx3;
            n = n3;
        }
    }

    if (n < arraymin)
        return (0);

    oned_t *o0 = 0, *oe = 0;
    int last = od->eltx;
    for (int i = 0; i < asize; i++) {
        if (!ary[i])
            continue;
        if (ary[i]->eltx == last) {
            last += d;
            oned_t *op = 0, *on;
            for (oned_t *o = ary[i]; o; o = on) {
                on = o->next;
                yb = o->elty;
                yt = yb + (o->nelts-1)*dy;

                if (yb <= minval && yt >= maxval) {
                    if (op)
                        op->next = on;
                    else
                        ary[i] = on;
                    o->next = 0;
                    if (yb < minval) {
                        int ne = (minval - yb)/dy;
                        if (ne >= runmin) {
                            oned_t *onew = new_oned(o->eltx, yb, 0, dy, ne);
                            if (op) {
                                onew->next = op->next;
                                op->next = onew;
                            }
                            else {
                                onew->next = ary[i];
                                ary[i] = onew;
                            }
                            op = onew;
                        }
                        else {
                            for (int y = yb; y < minval; y += dy) {
                                coor_t *p = unused;
                                unused = unused->next;
                                p->x = o->eltx;
                                p->y = y;
                                p->next = residual;
                                residual = p;
                            }
                        }
                    }
                    if (maxval < yt) {
                        int ne = (yt - maxval)/dy;
                        if (ne >= runmin) {
                            oned_t *onew = new_oned(o->eltx, maxval + dy,
                                0, dy, ne);
                            if (op) {
                                onew->next = op->next;
                                op->next = onew;
                            }
                            else {
                                onew->next = ary[i];
                                ary[i] = onew;
                            }
                            op = onew;
                        }
                        else {
                            for (int y = maxval + dy; y <= yt; y += dy) {
                                coor_t *p = unused;
                                unused = unused->next;
                                p->x = o->eltx;
                                p->y = y;
                                p->next = residual;
                                residual = p;
                            }
                        }
                    }
                    o->elty = minval;
                    o->nelts = (maxval - minval)/dy + 1;
                    if (o0) {
                        oe->next = o;
                        oe = o;
                    }
                    else
                        o0 = oe = o;
                    n--;
                    break;
                }
                if (yb >= maxval)
                    break;
                op = o;
            }
            if (!n)
                break;
        }
    }

    return (o0);
}

