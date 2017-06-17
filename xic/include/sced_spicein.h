
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
 $Id: sced_spicein.h,v 5.7 2015/04/18 17:03:13 stevew Exp $
 *========================================================================*/

#ifndef SCED_SPICEIN_H
#define SCED_SPICEIN_H


namespace {
    struct sKey
    {
        const char *k_key;      // device key
        int k_min_nodes;        // minimum number of nodes
        int k_max_nodes;        // maximum number of nodes
        int k_num_devs;         // number of device references
        bool k_val_only;        // put all text in value string
        const char *k_ndev;     // name of device or `n' device
        const char *k_pdev;     // name of `p' device
        sKey *k_next;           // table link - internal use
    };

    // Database for device keys.  This maps device names to devices
    // available in the device library.
    //
    struct sKeyDb
    {
        sKeyDb();

        sKey *new_key(const char*, int, int, int, bool, const char*,
            const char*);
        bool add_key(sKey*);
        const sKey *find_key(const char*);
        void dump(FILE *fp);

    private:
        sKey            *db_keys[26];  // a-z
        eltab_t<sKey>   db_eltab;
        strtab_t        db_strtab;
        bool            db_constr;
    };

    // List element for node token, also used for general text list.
    //
    struct sNodeLink
    {
        sNodeLink(char *n, sNodeLink *nx)
            {
                node = n;
                next = nx;
            }

        ~sNodeLink() { delete [] node; }

        void free()
            {
                sNodeLink *n = this;
                while (n) {
                    sNodeLink *x = n;
                    n = n->next;
                    delete x;
                }
            }

        char *node;
        sNodeLink *next;
    };

    // List global nodes to map.
    //
    struct sGlobNode
    {
        sGlobNode(char *n, char *s, sGlobNode *x)
            {
                nname = n;
                subst = s;
                next = x;
            }

        ~sGlobNode()
            {
                delete [] nname;
                delete [] subst;
            }

        void free()
            {
                sGlobNode *g = this;
                while (g) {
                    sGlobNode *x = g;
                    g = g->next;
                    delete x;
                }
            }

        char *nname;        // node name given
        char *subst;        // substitution
        sGlobNode *next;
    };

    // List element for model token.
    //
    struct sModLink
    {
        sModLink(char *mn, char *mt, char *tx, sModLink *n)
            {
                modname = mn;
                modtype = mt;
                text = tx;
                next = n;
            }

        ~sModLink()
            {
                delete [] modname;
                delete [] modtype;
                delete [] text;
            }

        void free()
            {
                sModLink *m = this;
                while (m) {
                    sModLink *x = m;
                    m = m->next;
                    delete x;
                }
            }

        char *modname;
        char *modtype;
        char *text;
        sModLink *next;
    };

    // List element for subcircuits.
    //
    struct sSubcLink
    {
        sSubcLink(char *n, sNodeLink *nds, int lev, sSubcLink *nxt)
            {
                name = n;
                nodes = nds;
                next = nxt;
                lines = lptr = 0;
                stab = 0;
                calls = 0;
                dotparams = 0;
                models = 0;
                owner = lev > 0 ? nxt : 0;
                done = false;
            }

        ~sSubcLink()
            {
                delete [] name;
                nodes->free();
                lines->free();
                delete stab;
                calls->free();
                dotparams->free();
                models->free();
            }

        void free()
            {
                sSubcLink *s = this;
                while (s) {
                    sSubcLink *x = s;
                    s = s->next;
                    delete x;
                }
            }

        // Add the names of referenced subcircuits to the 'calls' field.
        //
        void addcalls()
            {
                for (sNodeLink *n = lines; n; n = n->next) {
                    char *s = n->node;
                    while (isspace(*s))
                        s++;
                    if (*s == 'x' || *s == 'X') {
                        char *tok = 0;
                        do {
                            delete [] tok;
                            tok = lstring::gettok(&s);
                        } while (*s);
                        // tok should be the subcircuit name
                        calls = new sNodeLink(tok, calls);
                    }
                }
            }

        sSubcLink *next;
        char *name;            // subcircuit name
        sNodeLink *nodes;      // node arguments
        sNodeLink *lines;      // body text lines
        sNodeLink *lptr;       // pointer to text line
        sNodeLink *calls;      // list of subcircuit calls
        stringlist *dotparams; // .param lines in subcircuit text
        sModLink *models;      // .model lines in subcircuit text
        sSubcLink *owner;      // parent if subcircuit nested
        SymTab *stab;          // device name symbol table
        bool done;             // this subckt has been processed
    };

    // Class for sorting the subcircuit list, leaf to root.
    //
    struct sSortSubc
    {
        sSortSubc(sSubcLink *s)
            {
                subs = s;
                srtd = end = 0;
                while (subs)
                    recurse(subs);
            }

        sSubcLink *sorted() { return (srtd); }

        void add(sSubcLink *s0)
            {
                sSubcLink *sp = 0;
                for (sSubcLink *s = subs; s; s = s->next) {
                    if (s == s0) {
                        if (!sp)
                            subs = s->next;
                        else
                            sp->next = s->next;
                        if (!srtd)
                            srtd = end = s;
                        else {
                            end->next = s;
                            end = end->next;
                        }
                        return;
                    }
                    sp = s;
                }
            }

        sSubcLink *find(sNodeLink *n)
            {
                for (sSubcLink *s = subs; s; s = s->next) {
                    if (!strcmp(s->name, n->node))
                        return (s);
                }
                return (0);
            }

        void recurse(sSubcLink *s)
            {
                for (sNodeLink *n = s->calls; n; n = n->next) {
                    sSubcLink *xs = find(n);
                    if (xs)
                        recurse(xs);
                }
                add(s);
            }

    private:
        sSubcLink *subs;
        sSubcLink *srtd;
        sSubcLink *end;
    };

    struct sMut
    {
        sMut(char *na, char *l1, char *l2, char *v, sMut *nx)
            {
                name = na;
                ind1 = l1;
                ind2 = l2;
                val = v;
                next = nx;
            }

        ~sMut()
            {
                delete [] name;
                delete [] ind1;
                delete [] ind2;
                delete [] val;
            }

        void free()
            {
                sMut *m = this;
                while (m) {
                    sMut *x = m;
                    m = m->next;
                    delete x;
                }
            }

        char *name;
        char *ind1;
        char *ind2;
        char *val;
        sMut *next;
    };


    // SPICE processor.  This builds or updates a cell hierarchy based on
    // SPICE input.
    //
    namespace sced_spicein {
        struct cSpiceBuilder
        {
            friend void cSced::extractFromSpice(CDs*, FILE*, int);

            cSpiceBuilder();
            ~cSpiceBuilder();

            bool init_term_names();
            void add_node(char*);
            void add_dev(char*);
            const char *devname(const sKey *dev);
            char *nodename(int);
            sModLink *findmod(const CDs*, const char*, char**);
            sSubcLink *findsc(const char*);
            void make_subc(const char*, int);
            void place(CDs*, const char*, bool);
            bool apply_properties();
            void add_glob(const char*, const char*);
            void sub_glob(CDs*);
            bool apply_terms(CDs*);
            void cur_posn(CDs*, int*, int*);
            void cur_rpos(CDc*);
            void parseline(CDs*, const char*, bool);
            void process_muts(CDs*);
            bool dump_models(const char*);

            static char *readline(FILE*, int*);

            const char *modfile_name()  { return (sb_modfile); }

        private:
            void dump_models(FILE*, sModLink*, const char*);
            const char *setnp(const sKey*, char*, bool = false);
            int orient(int, int, const BBox*);
            void record_error(const char*);
            bool fix_empty_cell(CDs*);

            static void clear_cell(CDs*);
            static void mksymtab(CDs*, SymTab**, bool);

            char *sb_name;              // instance name
            char *sb_model;             // model property text
            char *sb_value;             // value property text
            char *sb_param;             // param property text
            sNodeLink *sb_nodes;        // list of node tokens
            sNodeLink *sb_devs;         // list of device reference tokens
            sModLink *sb_models;        // list of models
            sSubcLink *sb_subckts;      // list of subcircuits
            sGlobNode *sb_globs;        // global nodes
            CDc *sb_cdesc;              // pointer to created instance
            sNodeLink *sb_lines;        // text
            SymTab *sb_stab;            // device name symbol table
            sMut *sb_mutlist;           // list of mutual inductors
            sKeyDb *sb_keydb;           // device mapping database
            char *sb_term_name;         // library terminal dev name
            char *sb_gnd_name;          // library ground dev name
            char *sb_modfile;           // model file name when written

            int sb_xpos;                // x coordinate for placement
            int sb_ypos;                // y coordinate for placement
            int sb_lineht;              // height of cell row
        };
    }
}

#endif

