
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

#ifndef FIO_LAYERMAP_H
#define FIO_LAYERMAP_H


//-----------------------------------------------------------------------------
// Struct to hold layer filtering info.
//
struct FIOlayerFilt
{
    FIOlayerFilt()
        {
            lf_layer_list = 0;
            lf_aliases = 0;
            lf_use_only = false;
            lf_skip = false;
        }

    ~FIOlayerFilt()
        {
            delete [] lf_layer_list;
            delete [] lf_aliases;
        }

    void set();
    void push();

    const char *layer_list()        const { return (lf_layer_list); }
    const char *aliases()           const { return (lf_aliases); }
    bool use_only()                 const { return (lf_use_only); }
    bool skip()                     const { return (lf_skip); }

    void set_layer_list(char *ll)   { delete [] lf_layer_list;
                                      lf_layer_list = ll; }
    void set_aliases(char *la)      { delete [] lf_aliases;
                                      lf_aliases = la; }
    void set_use_only(bool b)       { lf_use_only = b; }
    void set_skip(bool b)           { lf_skip = b; }

private:
    char *lf_layer_list;
    char *lf_aliases;
    bool lf_use_only;
    bool lf_skip;
};


//-----------------------------------------------------------------------------
// Layer name aliasing table
//

// Table item.
//
struct al_t
{
    const char *tab_name()      { return (name); }
    void set_tab_name(const char *n) { name = n; }
    al_t *tab_next()            { return (next); }
    void set_tab_next(al_t *t)  { next = t; }
    al_t *tgen_next(bool)       { return (next); }

    const char *tab_alias()     { return (alias); }
    void set_tab_alias(const char *a) { alias = a; }

private:
    const char *name;
    const char *alias;
    al_t *next;
};

// Layer alias table.
//
struct FIOlayerAliasTab
{
    FIOlayerAliasTab()          { at_table = 0; }
    ~FIOlayerAliasTab()         { delete at_table; }

    bool addAlias(const char*, const char*);
    void remove(const char*);
    void clear();
    void parse(const char*);
    void readFile(FILE*);
    char *toString(bool);
    void dumpFile(FILE*);

    const char *alias(const char *name)
        {
            if (!at_table)
                return (0);
            al_t *e = at_table->find(name);
            return (e ? e->tab_alias() : 0);
        }

private:
    void add(const char *name, const char *new_name)
        {
            if (!name || !*name || !new_name || !*new_name)
                return;
            if (!at_table)
                at_table = new table_t<al_t>;
            al_t *e = at_table->find(name);
            if (!e) {
                e = at_eltab.new_element();
                e->set_tab_name(at_stringtab.add(name));
                e->set_tab_next(0);
                at_table->link(e, false);
                at_table = at_table->check_rehash();
            }
            e->set_tab_alias(at_stringtab.add(new_name));
        }

    table_t<al_t> *at_table;
    eltab_t<al_t> at_eltab;
    strtab_t at_stringtab;
};

#endif

