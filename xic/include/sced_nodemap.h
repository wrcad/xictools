
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

#ifndef SCED_NODEMAP_H
#define SCED_NODEMAP_H


// List element for assigned node name.
//
struct sNodeName
{
    sNodeName(CDnetName na, hyEnt *h, sNodeName *ne)
        {
            nn_next = ne;
            nn_name = na;
            nn_hent = h;
        }
    ~sNodeName();

    static void destroy(sNodeName *n) {
        while (n) {
            sNodeName *nx = n;
            n = n->nn_next;
            delete nx;
        }
    }

    sNodeName *next()       const { return  (nn_next); }
    void set_next(sNodeName *n)   { nn_next = n; }

    CDnetName name()        const { return (nn_name); }
    void set_name(CDnetName n)    { nn_name = n; }

    hyEnt *hent()           const { return (nn_hent); }

private:
    sNodeName *nn_next;
    CDnetName nn_name;          // alias
    hyEnt *nn_hent;             // link to underlying node
};

// List element, contains a name string and x,y location.  Returned by
// cNodeMap::getSetList.
//
struct xyname_t
{
    xyname_t(CDnetName nm, int x, int y)
        {
            xy_next = 0;
            xy_name = nm;
            xy_x = x;
            xy_y = y;
        }

    static void destroy(xyname_t *n)
        {
            while (n) {
                xyname_t *nx = n;
                n = n->xy_next;
                delete nx;
            }
        }

    int posx()          const { return (xy_x); }
    int posy()          const { return (xy_y); }
    const char *name()  const { return ((const char*)xy_name); }
    xyname_t *next()    const { return (xy_next); }

    void set_next(xyname_t *n)  { xy_next = n; }

private:
    xyname_t *xy_next;
    CDnetName xy_name;
    int xy_x;
    int xy_y;
};

// cNodeMap::nm_fmap element flags
#define NM_SET  0x1
    // Name is user-set.
#define NM_GLOB 0x2
    // Name is a global node name.

// Map for naming nodes.  A pointer to this struct is used in the CDs
// sGroups field of electrical structures.
//
class cNodeMap
{
public:
    cNodeMap(CDs*);
    ~cNodeMap()
        {
            delete [] nm_nmap;
            delete [] nm_fmap;
            sNodeName::destroy(nm_setnames);
            delete nm_netname_tab;
        }

    void setDirty()     { nm_dirty = true; }
    bool isDirty()      const { return (nm_dirty); }

    int findNode(const char*);
    int findNode(CDnetName);
    int countNodes();

    void setupNetNames(int, SymTab*);
    bool newEntry(const char*, int);
    void delEntry(int);
    const char *map(int) const;
    CDnetName mapStab(int) const;
    const char *mapName(int) const;
    bool isSet(int) const;
    bool isGlobal(int) const;
    int hasGlobal(bool = false) const;
    void tabAddGlobal(SymTab*) const;
    void updateProperty();
    xyname_t *getSetList() const;

private:
    void extract_setnames();
    void setup();
    void refresh();

    CDnetName *nm_nmap;          // map to name strings
    unsigned char *nm_fmap;      // map to flags
    sNodeName *nm_setnames;      // list of assigned names
    CDs *nm_celldesc;            // back pointer to cell
    SymTab *nm_netname_tab;      // name/node table from connection op.
    int nm_size;                 // actual size of map
    int nm_connect_size;         // size from connection operation
    bool nm_dirty;               // map needs refresh
};

#endif

