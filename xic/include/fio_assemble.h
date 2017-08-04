
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

#ifndef FIO_ASSEMBLE_H
#define FIO_ASSEMBLE_H


#include "fio_layermap.h"

class cCHD;
struct Topcell;
struct FIOaliasTab;


// Struct describing an instance to be placed in the topcell.
struct ainst_t
{
    ainst_t(char *n, ainst_t *inst)
        {
            next = 0;
            i_cellname = n;
            i_placename = 0;
            i_refinst = 0;
            i_tx = inst ? inst->i_tx : 0;
            i_ty = inst ? inst->i_ty : 0;
            i_ang = inst ? inst->i_ang : 0.0;
            i_magn = inst ? inst->i_magn : 1.0;
            i_reflc = inst ? inst->i_reflc : false;
            i_no_hier = inst ? inst->i_no_hier : false;
            set_scale(inst ? inst->scale() : 1.0);
            if (inst && inst->use_win())
                set_winBB(inst->winBB());
            set_ecf_level(inst ? inst->ecf_level() : ECFnone);
            set_flatten(inst ? inst->flatten() : false);
            set_use_win(inst ? inst->use_win() : false);
            set_clip(inst ? inst->clip() : false);
        }

    ainst_t(char *n, int x, int y, bool my, double r, double m, double s,
            bool nh, ECFlevel o, bool f, BBox *bb, bool c)
        {
            next = 0;
            i_cellname = n;
            i_placename = 0;
            i_refinst = 0;
            i_tx = x;
            i_ty = y;
            i_ang = r;
            i_magn = m;
            i_reflc = my;
            i_no_hier = nh;
            set_scale(s);
            set_ecf_level(o);
            set_flatten(f);
            if (bb) {
                set_winBB(bb);
                set_use_win(true);
            }
            else
                set_use_win(false);
            set_clip(c);
        }

    bool is_similar(ainst_t *a)
        {
            return (
                !strcmp(i_cellname, a->i_cellname) &&
                (i_placename == a->i_placename ||
                    (i_placename && a->i_placename &&
                    !strcmp(i_placename, a->i_placename))) &&
                ecf_level() == a->ecf_level() &&
                fabs(scale() - a->scale()) < 1e-12 &&
                flatten() == a->flatten() &&
                no_hier() == a->no_hier() &&
                use_win() == a->use_win() &&
                (!use_win() || *winBB() == *a->winBB()) &&
                clip() == a->clip());
        }

    ~ainst_t()
        {
            delete [] i_cellname;
            delete [] i_placename;
        }

    ainst_t *next_instance()    const { return (next); }
    const char *cellname()      const { return (i_cellname); }
    const char *placename()     const { return (i_placename); }
    ainst_t *refinst()          const { return (i_refinst); }
    int pos_x()                 const { return (i_tx); }
    int pos_y()                 const { return (i_ty); }
    double angle()              const { return (i_ang); }
    double magn()               const { return (i_magn); }
    bool reflc()                const { return (i_reflc); }
    bool no_hier()              const { return (i_no_hier); }
    double scale()              const { return (i_prms.scale()); }
    const BBox *winBB()         const { return (i_prms.window()); }
    ECFlevel ecf_level()        const { return (i_prms.ecf_level()); }
    bool flatten()              const { return (i_prms.flatten()); }
    bool use_win()              const { return (i_prms.use_window()); }
    bool clip()                 const { return (i_prms.clip()); }

    void set_next_instance(ainst_t *i)  { next = i; }
    void set_cellname(char *c)          { delete [] i_cellname;
                                          i_cellname = c; }
    void set_placename(char *p)         { delete [] i_placename;
                                          i_placename = p; }
    void set_refinst(ainst_t *i)        { i_refinst = i; }
    void set_pos_x(int x)               { i_tx = x; }
    void set_pos_y(int y)               { i_ty = y; }
    void set_angle(double a)            { i_ang = a; }
    void set_magn(double m)             { i_magn = m; }
    void set_reflc(bool b)              { i_reflc = b; }
    void set_no_hier(bool b)            { i_no_hier = b; }
    void set_scale(double m)            { i_prms.set_scale(m); }
    void set_winBB(const BBox *BB)      { i_prms.set_window(BB); }
    void set_ecf_level(ECFlevel f)      { i_prms.set_ecf_level(f); }
    void set_flatten(bool b)            { i_prms.set_flatten(b); }
    void set_use_win(bool b)            { i_prms.set_use_window(b); }
    void set_clip(bool b)               { i_prms.set_clip(b); }

    const FIOcvtPrms *prms()            { return (&i_prms); }

private:
    ainst_t *next;              // link
    char *i_cellname;           // instance cell name
    char *i_placename;          // name instance will be placed as
    ainst_t *i_refinst;         // pointer to similar instance
    int i_tx, i_ty;             // translation
    double i_ang;               // rotation
    double i_magn;              // instance magnification
    FIOcvtPrms i_prms;          // conversion parameters
    bool i_reflc;               // reflection
    bool i_no_hier;             // this cell only, no hierarchy
};

// Specification for an archive source.
struct asource_t
{
    asource_t(char *fn, asource_t *src)
        {
            s_scale = src ? src->s_scale : 1.0;
            next = 0;
            s_path = fn;
            s_prefix = src ? lstring::copy(src->s_prefix) : 0;
            s_suffix = src ? lstring::copy(src->s_suffix) : 0;
            s_instances = 0;
            s_chd = 0;
            if (src) {
                s_lf.set_layer_list(lstring::copy(src->layer_list()));
                s_lf.set_aliases(lstring::copy(src->layer_aliases()));
                s_lf.set_use_only(src->only_layers());
                s_lf.set_skip(src->skip_layers());
            }
            s_tolower = src ? src->s_tolower : false;
            s_toupper = src ? src->s_toupper : false;
        }

    ~asource_t()
        {
            delete [] s_path;
            delete [] s_prefix;
            delete [] s_suffix;
            while (s_instances) {
                ainst_t *inst = s_instances;
                s_instances = s_instances->next_instance();
                delete inst;
            }
        }

    bool set_match_chd(cCHD*);

    double scale()              const { return (s_scale); }
    asource_t *next_source()    const { return (next); }
    const char *path()          const { return (s_path); }
    const char *layer_list()    const { return (s_lf.layer_list()); }
    const char *layer_aliases() const { return (s_lf.aliases()); }
    const char *prefix()        const { return (s_prefix); }
    const char *suffix()        const { return (s_suffix); }
    ainst_t *instances()        const { return (s_instances); }
    cCHD *chd()                 const { return (s_chd); }
    bool only_layers()          const { return (s_lf.use_only()); }
    bool skip_layers()          const { return (s_lf.skip()); }
    bool to_lower()             const { return (s_tolower); }
    bool to_upper()             const { return (s_toupper); }

    void set_scale(double s)            { s_scale = s; }
    void set_next_source(asource_t *n)  { next = n; }
    void set_path(char *p)              { delete [] s_path; s_path = p; }
    void set_layer_list(char *ll)       { s_lf.set_layer_list(ll); }
    void set_layer_aliases(char *la)    { s_lf.set_aliases(la); }
    void set_prefix(char *p)            { delete [] s_prefix; s_prefix = p; }
    void set_suffix(char *s)            { delete [] s_suffix; s_suffix = s; }
    void set_instances(ainst_t *i)      { s_instances = i; }
    void set_chd(cCHD *c)               { s_chd = c; }
    void set_only_layers(bool b)        { s_lf.set_use_only(b); }
    void set_skip_layers(bool b)        { s_lf.set_skip(b); }
    void set_to_lower(bool b)           { s_tolower = b; }
    void set_to_upper(bool b)           { s_toupper = b; }

    void push_layer_alias()             { s_lf.push(); }

private:
    double s_scale;             // conversion scale factor
    asource_t *next;
    char *s_path;               // path to archive
    char *s_prefix;             // cell name substitution prefix
    char *s_suffix;             // cell name substitution suffix
    ainst_t *s_instances;       // list of instances for top cell
    cCHD *s_chd;                // cell hierarchy digest handle if provided
    FIOlayerFilt s_lf;          // layer filtering info
    bool s_tolower;             // cell names to lower case
    bool s_toupper;             // cell names to upper case
};

// Struct describing the job
struct ajob_t
{
    ajob_t(const char *o)
        {
            j_topcell = 0;
            j_outfile = lstring::copy(o);
            j_logfile = 0;
            j_srcfiles = 0;
            j_prms = 0;
            j_out = 0;
            j_alias = 0;
        }
    ~ajob_t();

    bool open_stream();
    bool add_topcell(const char*);
    bool add_source(const char*, cCHD*, double, const char*, const char*,
        const char*, const char*, const char*, int);
    bool add_instance(const char*, int, int, bool, double, double, double,
        bool, ECFlevel, bool, BBox*, bool);
    bool write_stream(asource_t*);
    bool finish_stream();

    bool parse(FILE*);
    bool parse(const char*);
    bool run(const char*);
    bool dump(FILE*);

    asource_t *sources() { return (j_srcfiles); }

    const char *topcell()       const { return (j_topcell); }
    const char *outfile()       const { return (j_outfile); }
    const asource_t *sources()  const { return (j_srcfiles); }

private:
    Topcell *new_topcell();

    char *j_topcell;            // name of top-level cell to create
    char *j_outfile;            // name of output file
    char *j_logfile;            // name of log file
    asource_t *j_srcfiles;      // list of input files

    FIOcvtPrms *j_prms;         // output conversion desc
    cv_out *j_out;              // conversion output context
    FIOaliasTab *j_alias;       // alias table transfer
};

#endif

