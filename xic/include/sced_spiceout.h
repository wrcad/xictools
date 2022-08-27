
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

#ifndef SCED_SPICEOUT_H
#define SCED_SPICEOUT_H

//
// Struct for generating SPICE output.
//

class SpOut
{
public:
    struct sp_alias_t
    {
        sp_alias_t() { name = 0; alias = 0; next = 0; }

        char *name;
        char *alias;
        sp_alias_t *next;
    };

    struct sp_nmlist_t
    {
        sp_nmlist_t(int n, char *na, sp_nmlist_t *x)
            {
                node = n;
                name = na;
                next = x;
            }
        ~sp_nmlist_t() { delete [] name; }


        static const char *globname(const sp_nmlist_t* thisn, int num)
            {
                if (num < 1)
                    return (0);
                for (const sp_nmlist_t *m = thisn; m; m = m->next) {
                    if (m->node == num)
                        return (m->name);
                }
                return (0);
            }

        static void destroy(sp_nmlist_t *n)
            {
                while (n) {
                    sp_nmlist_t *nx = n;
                    n = n->next;
                    delete nx;
                }
            }

        int node;
        char *name;
        sp_nmlist_t *next;
    };

    // For spicetext lines from labels.
    struct sp_stl_t
    {
        int n;
        char *str;
    };

    // This class obtains the device names used in .save lines
    //
    struct sp_dsave_t
    {
        sp_dsave_t()
            {
                ds_saves = 0;
                ds_sp = 0;
                memset(ds_names, 0, CDMAXCALLDEPTH*sizeof(char*));
            }

        ~sp_dsave_t()
            {
                for (ds_sp--; ds_sp >= 0; ds_sp--)
                    delete [] ds_names[ds_sp];
                SpiceLine::destroy(ds_saves);
            }

        SpiceLine *lines()      { return (ds_saves); }

        void process(CDs*, const char*);

    private:
        bool push(const char*);
        void pop();
        void save(const char*);

        SpiceLine *ds_saves;    // List of .saves.
        int ds_sp;              // stack pointer
        char *ds_names[CDMAXCALLDEPTH];
                                // subcircuit call stack
    };

    SpOut(CDs*);
    ~SpOut()                    { free_alias(); }

    void setSkipNoPhys(bool b)  { sp_skip_nophys = b; }
    bool skipNoPhys()           { return (sp_skip_nophys); }

    SpiceLine *makeSpiceDeck(SymTab** = 0, bool = false);

private:
    SpiceLine *ckt_deck(CDs*, bool = false, bool = false);
    sp_nmlist_t *def_node_term_list(CDs*);
    stringlist *list_def_node_names();
    SpiceLine *add_mutual(CDs*, bool, stringlist**, int);
    SpiceLine *add_dot_save(const char*, char**);
    SpiceLine *add_dot_global();
    SpiceLine *add_dot_include();
    SpiceLine *assert_lib_properties(SpiceLine*);
    void add_global_node(sLstr*, int, CDc*, const sp_nmlist_t*);
    SpiceLine *subckt_line(CDs*);
    SpiceLine *subcircuits(CDs*);
    SymTab *subcircuits_tab(CDs*);
    void subc_list(CDs*, SymTab*);
    SpiceLine *get_sptext_labels(CDs*);
    void spice_deck_sort(SpiceLine*);
    void check_dups(SpiceLine*, CDs*);
    void read_alias();
    sp_alias_t *get_alias(const char**);
    char *subst_alias(char*);
    void free_alias();

    sp_alias_t *sp_alias_list;
    const stringlist *sp_properties;
    CDs *sp_celldesc;
    bool sp_skip_nophys;
};

#endif

