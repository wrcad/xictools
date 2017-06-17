
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
 $Id: sced_spiceout.h,v 5.12 2012/12/06 01:27:57 stevew Exp $
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


        char *globname(int num)
            {
                if (num < 1)
                    return (0);
                for (sp_nmlist_t *m = this; m; m = m->next) {
                    if (m->node == num)
                        return (m->name);
                }
                return (0);
            }

        void free()
            {
                sp_nmlist_t *nxt;
                for (sp_nmlist_t *m = this; m; m = nxt) {
                    nxt = m->next;
                    delete m;
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
                ds_saves->free();
            }

        sp_line_t *lines()      { return (ds_saves); }

        void process(CDs*, const char*);

    private:
        bool push(const char*);
        void pop();
        void save(const char*);

        sp_line_t *ds_saves;    // List of .saves.
        int ds_sp;              // stack pointer
        char *ds_names[CDMAXCALLDEPTH];
                                // subcircuit call stack
    };

    SpOut(CDs*);
    ~SpOut()                    { free_alias(); }

    void setSkipNoPhys(bool b)  { sp_skip_nophys = b; }
    bool skipNoPhys()           { return (sp_skip_nophys); }

    sp_line_t *makeSpiceDeck(SymTab** = 0, bool = false);

private:
    sp_line_t *ckt_deck(CDs*, bool = false);
    sp_nmlist_t *def_node_term_list(CDs*);
    stringlist *list_def_node_names();
    sp_line_t *add_mutual(CDs*, bool, stringlist**, int);
    sp_line_t *add_dotsave(const char*, char**);
    sp_line_t *add_dot_global();
    sp_line_t *assert_lib_properties(sp_line_t*);
    void add_global_node(sLstr*, int, CDc*, const sp_nmlist_t*);
    sp_line_t *subckt_line(CDs*);
    sp_line_t *subcircuits(CDs*);
    SymTab *subcircuits_tab(CDs*);
    void subc_list(CDs*, SymTab*);
    sp_line_t *get_sptext_labels(CDs*);
    void spice_deck_sort(sp_line_t*);
    void check_dups(sp_line_t*, CDs*);
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

