
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

#ifndef CD_LGEN_H
#define CD_LGEN_H


//
// Generators for iterating over layers.
//

// Generator for layer descs in the layer table.  By default, order
// is bottom-up, skipping the cell layer.
//
struct CDlgen
{
    enum Mode {
        BotToTopNoCells,
        BotToTopWithCells,
        TopToBotNoCells,
        TopToBotWithCells
    };

    CDlgen(DisplayMode m, Mode set = BotToTopNoCells)
        {
            lg_mode = m;
            int nused = CDldb()->layersUsed(m);

            switch (set) {
            default:
            case BotToTopNoCells:
                lg_current = lg_start = 1;
                lg_end = nused - 1;
                break;
            case BotToTopWithCells:
                lg_current = lg_start = 0;
                lg_end = nused - 1;
                break;
            case TopToBotNoCells:
                lg_current = lg_start = nused - 1;
                lg_end = 1;
                break;
            case TopToBotWithCells:
                lg_current = lg_start = nused - 1;
                lg_end = 0;
                break;
            }
        }

    CDl *next()
        {
            if (lg_end >= lg_start) {
                if (lg_current > lg_end)
                    return (0);
            }
            else {
                if (lg_current < lg_end)
                    return (0);
            }
            CDl *ld = CDldb()->layer(lg_current, lg_mode);
            if (lg_end >= lg_start)
                lg_current++;
            else
                lg_current--;
            return (ld);
        }

private:
    int lg_start;
    int lg_end;
    int lg_current;
    DisplayMode lg_mode;
};

// A generator that includes the derived layers.  These follow the
// normal layers which are emitted in the default sequence.  The
// derived layers are returned in ascending order of the index
// numbers.
//
struct CDlgenDrv : public CDlgen
{
    CDlgenDrv() : CDlgen(Physical)
        {
            lg_cnt = -1;
            lg_ary = 0;
        }

    CDl *next()
        {
            if (lg_cnt < 0) {
                CDl *ld = CDlgen::next();
                if (ld)
                    return (ld);
                lg_ary = CDldb()->listDerivedLayers();
                if (lg_ary)
                    lg_cnt = 0;
            }
            if (lg_ary) {
                CDl *ld = lg_ary[lg_cnt++];
                if (!ld)  {
                    delete [] lg_ary;
                    lg_ary = 0;
                    lg_cnt = -1;
                    return (0);
                }
                return (ld);
            }
            return (0);
        }

private:
    CDl **lg_ary;
    int lg_cnt;
};

// Database for use with CDextLgen below.
//
struct CDextLtab
{
    CDextLtab()
        {
            el_conductors = 0;
            el_routing = 0;
            el_contacts = 0;
            el_vias = 0;
            el_n_routing = 0;
            el_n_conductors = 0;
            el_n_contacts = 0;
            el_n_vias = 0;

            {
                CDlgen lgen(Physical);
                CDl *ld;
                while ((ld = lgen.next()) != 0) {
                    if (ld->isConductor())
                        el_n_conductors++;
                    if (ld->isRouting())
                        el_n_routing++;
                    if (ld->isInContact())
                        el_n_contacts++;
                    if (ld->isVia())
                        el_n_vias++;
                }
            }
            if (el_n_conductors)
                el_conductors = new CDl*[el_n_conductors];
            if (el_n_routing)
                el_routing = new CDl*[el_n_routing];
            if (el_n_contacts)
                el_contacts = new CDl*[el_n_contacts];
            if (el_n_vias)
                el_vias = new CDl*[el_n_vias];
            el_n_conductors = 0;
            el_n_routing = 0;
            el_n_contacts = 0;
            el_n_vias = 0;
            {
                CDlgen lgen(Physical);
                CDl *ld;
                while ((ld = lgen.next()) != 0) {
                    if (ld->isConductor())
                        el_conductors[el_n_conductors++] = ld;
                    if (ld->isRouting())
                        el_routing[el_n_routing++] = ld;
                    if (ld->isInContact())
                        el_contacts[el_n_contacts++] = ld;
                    if (ld->isVia())
                        el_vias[el_n_vias++] = ld;
                }
            }
        }

    ~CDextLtab()
        {
            delete [] el_conductors;
            delete [] el_routing;
            delete [] el_contacts;
            delete [] el_vias;
        }

    CDl *conductors(unsigned int i)      const
        { return (i < el_n_conductors ? el_conductors[i] : 0); }
    CDl *routing(unsigned int i)         const
        { return (i < el_n_routing ? el_routing[i] : 0); }
    CDl *contacts(unsigned int i)        const
        { return (i < el_n_contacts ? el_contacts[i] : 0); }
    CDl *vias(unsigned int i)            const
        { return (i < el_n_vias ? el_vias[i] : 0); }

    int num_conductors()    const { return (el_n_conductors); }
    int num_routing()       const { return (el_n_routing); }
    int num_contacts()      const { return (el_n_contacts); }
    int num_vias()          const { return (el_n_vias); }

private:
    CDl **el_conductors;
    CDl **el_routing;
    CDl **el_contacts;
    CDl **el_vias;
    unsigned int el_n_conductors;
    unsigned int el_n_routing;
    unsigned int el_n_contacts;
    unsigned int el_n_vias;
};

// A generator for the extraction system, iterates over types of
// layers, but maintains a cache which should be speedier.
//
struct CDextLgen
{
    enum Mode { BotToTop, TopToBot };

    CDextLgen(int type, Mode m = BotToTop)
        {
            if (eg_dirty || !eg_ltab) {
                eg_ltab = new CDextLtab;
                eg_dirty = false;
            }
            eg_mode = m;
            eg_type = type;
            if (m == BotToTop)
                eg_index = 0;
            else if (type == CDL_CONDUCTOR)
                eg_index = eg_ltab->num_conductors() - 1;
            else if (type == CDL_ROUTING)
                eg_index = eg_ltab->num_routing() - 1;
            else if (type == CDL_IN_CONTACT)
                eg_index = eg_ltab->num_contacts() - 1;
            else if (type == CDL_VIA)
                eg_index = eg_ltab->num_vias() - 1;
            else
                eg_index = 0;
        }

    CDl *next()
        {
            if (eg_type == CDL_CONDUCTOR) {
                if (eg_mode == TopToBot) {
                    if (eg_index < 0)
                        return 0;
                    return (eg_ltab->conductors(eg_index--));
                }
                else {
                    if (eg_index >= eg_ltab->num_conductors())
                        return (0);
                    return (eg_ltab->conductors(eg_index++));
                }
            }
            else if (eg_type == CDL_ROUTING) {
                if (eg_mode == TopToBot) {
                    if (eg_index < 0)
                        return 0;
                    return (eg_ltab->routing(eg_index--));
                }
                else {
                    if (eg_index >= eg_ltab->num_routing())
                        return (0);
                    return (eg_ltab->routing(eg_index++));
                }
            }
            else if (eg_type == CDL_IN_CONTACT) {
                if (eg_mode == TopToBot) {
                    if (eg_index < 0)
                        return 0;
                    return (eg_ltab->contacts(eg_index--));
                }
                else {
                    if (eg_index >= eg_ltab->num_contacts())
                        return (0);
                    return (eg_ltab->contacts(eg_index++));
                }
            }
            else if (eg_type == CDL_VIA) {
                if (eg_mode == TopToBot) {
                    if (eg_index < 0)
                        return 0;
                    return (eg_ltab->vias(eg_index--));
                }
                else {
                    if (eg_index >= eg_ltab->num_vias())
                        return (0);
                    return (eg_ltab->vias(eg_index++));
                }
            }
            return (0);
        }

    // Call this to force an update of the layer cache.
    static void set_dirty()     { eg_dirty = true; }

    static CDextLtab *ext_ltab()
        {
            if (eg_dirty || !eg_ltab) {
                delete eg_ltab;
                eg_ltab = new CDextLtab;
                eg_dirty = false;
            }
            return (eg_ltab);
        }

private:
    int eg_index;
    unsigned int eg_mode;
    unsigned int eg_type;

    static CDextLtab *eg_ltab;
    static bool eg_dirty;
};

// Generator to cycle through the layers used in a cell.  These are
// NOT in layer-table order unless the sort method is called, before
// any retrieval.
//
struct CDsLgen
{
    CDsLgen(const CDs *sd, bool with_cell_layer = false)
        {
            g_tree = 0;
            g_descs = 0;
            g_indx = 0;
            g_size = 0;
            g_with_cell = with_cell_layer;
            g_mode = Physical;
            if (sd) {
                g_tree = sd->layer_heads();
                g_size = sd->num_layers_used();
                g_mode = sd->displayMode();
            }
        }

    ~CDsLgen()
        {
            delete [] g_descs;
        }

    // Note that ld->index() < 0 are never returned, these are not
    // in the layer table.

    CDl *next()
        {
            while (g_indx < g_size) {
                if (g_descs)
                    return (g_descs[g_indx++]);
                const CDtree *rt = g_tree + g_indx++;
                if (rt->num_elements()) {
                    CDl *ld = rt->ldesc();
                    if (ld && ((ld->index(g_mode) > 0) ||
                            (g_with_cell && ld->index(g_mode) == 0)))
                        return (ld);
                }
            }
            return (0);
        }

    // Sort into layer-table order (cd_layer.cc).
    void sort();

private:
    const CDtree *g_tree;   // List of CDtree elements from cell.
    CDl **g_descs;          // Sorted layer list, from sort method.
    int g_indx;             // Current index.
    int g_size;             // Ending index.
    bool g_with_cell;       // Also return cell layer if true.
    DisplayMode g_mode;     // Display mode of cell.
};

#endif

