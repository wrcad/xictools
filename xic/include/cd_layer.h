
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

#ifndef CD_LAYER_H
#define CD_LAYER_H


struct strm_idata;
struct strm_odata;


// Flags for CDl::ld_flags.
// All application flags are defined here, with set/unset functions.
// Few of these are actually used in CD.
//
#define CDL_WIRE            0x00000001  // elect. active wire in schematic
#define CDL_IMMUT           0x00000002  // prevent object add/remove
#define CDL_NOINSTVW        0x00000004  // invisible in instances
#define CDL_SYMBOLIC        0x00000008  // layer is symbolic
#define CDL_INVIS           0x00000010  // layer is currently invisible
#define CDL_RSTINVIS        0x00000020  // layer set invisible in tech file
#define CDL_NOSELECT        0x00000040  // don't select objects on this layer
#define CDL_RSTNOSEL        0x00000080  // layer set noselect in tech file
#define CDL_NOMERGE         0x00000100  // don't automatically merge
#define CDL_NODRC           0x00000200  // no DRC on this layer
#define CDL_INVALID         0x00000400  // invalid, not shown in layer table
#define CDL_SKIP            0x00000800  // temporary skip flag
#define CDL_USEDT1          0x00001000  // datatype mapping
#define CDL_USEDT2          0x00002000  // datatype mapping
#define CDL_USEDT3          0x00004000  // datatype mapping

// Flags for presentation attributes.
#define CDL_BLINK           0x00008000   // blinking layer
#define CDL_CUT             0x00010000   // boxes drawn with X
#define CDL_OUTLINED        0x00020000   // outline style bit 1
#define CDL_OUTLINEDFAT     0x00040000   // outline style bit 2
#define CDL_FILLED          0x00080000   // layer is filled

// Mask for flags that might be changed in hard-copy.  This includes
// the flags in the group above, plus the CDL_INVIS flag.
//
#define CDL_ATTR_FLAGS     (0x000f8000 | CDL_INVIS)

// Flags for extraction system.
#define CDL_CONDUCTOR       0x00100000   // layer is wire net
#define CDL_ROUTING         0x00200000   // indicates a routing layer
#define CDL_GROUNDPLANE     0x00400000   // ground plane layer
#define CDL_IN_CONTACT      0x00800000   // layer will contact another net
#define CDL_VIA             0x01000000   // layer is via between nets
#define CDL_VIACUT          0x02000000   // layer dielectric with cuts defined
#define CDL_DIELECTRIC      0x04000000   // layer is capacitor dielectric
#define CDL_PLANARIZE       0x08000000   // layer planarizes, y/n given
#define CDL_DARKFIELD       0x10000000   // layer is polarity reversed

// Flags in the ld_flags2 field.
#define CDLPRV_TYPE         0x3         // the first 2 bits specify the
                                        // layer type
#define CDLPRV_ELECSTD      0x4         // standard electrical layer
#define CDLPRV_PLZASSET     0x8         // planarizing set to this state

// Layer type.
enum CDLtype { CDLnormal, CDLderived, CDLinternal, CDLcellInstance };

// CDLnormal
//   Normal layer.
// CDLderived
//   Objects are constructed with a user-supplied layer expression
//   involving other layers.  These are generally for DRC but can be
//   used for other purposes.  They are not written out to cell
//   storage.  They have normal layer names and colors, which can be
//   saved in a technology database as a derived layer.
// CDLinternal
//   Like a normal layer, except that it will never be written out to
//   tech or cell storage.  Some operations (e.g., listings of layers
//   used) may ignore these.  These layers are generally created
//   internally for temporary use.  The names generally begin with '$'.
// CDLcellInstance
//   Like internal layers, but always invisible.  These are used only
//   for cell instance layers, i.e., the pseudo-layer where cell
//   instance locations are recorded.
//

// Flags for derived layer creation.
//
// Mask and join/split vals, there are used in the ld_drv_mode field.
#define CLmode      0x3
#define CLdefault   0x0
#define CLsplitH    0x1
#define CLsplitV    0x2
#define CLjoin      0x3
//
// Flags.
#define CLnoClear   0x4
#define CLmerge     0x8
#define CLnoUndo    0x10
#define CLrecurse   0x20


// Layer descriptor.
//
struct CDl
{
    friend class cCDldb;
    friend struct CDlary;

    CDl(CDLtype);
    ~CDl();

    const char *oaLayerName() const;
    const char *oaPurposeName() const;
    void addStrmOut(unsigned int, unsigned int);
    bool getStrmOut(unsigned int*, unsigned int*, const CDo* = 0);
    int clearStrmOut(unsigned int, unsigned int);
    unsigned int getStrmNoDrcDatatype(const CDo*);
    bool setStrmIn(const char*);
    bool setStrmIn(unsigned int, unsigned int);
    bool isStrmIn(unsigned int, unsigned int);
    int clearStrmIn();
    int getStrmDatatypeFlags(int);

    static char *get_layer_tok(const char**);

    CDLtype layerType() const
        {
            return ((CDLtype)(ld_flags2 & CDLPRV_TYPE));
        }

    // Get flags.
    unsigned int flags()        const { return (ld_flags); }
    bool isWireActive()         const { return (ld_flags & CDL_WIRE); }
    bool isImmutable()          const { return (ld_flags & CDL_IMMUT); }
    bool isNoInstView()         const { return (ld_flags & CDL_NOINSTVW); }
    bool isSymbolic()           const { return (ld_flags & CDL_SYMBOLIC); }
    bool isInvisible()          const { return (ld_flags & CDL_INVIS); }
    bool isRstInvisible()       const { return (ld_flags & CDL_RSTINVIS); }
    bool isNoSelect()           const { return (ld_flags & CDL_NOSELECT); }
    bool isRstNoSelect()        const { return (ld_flags & CDL_RSTNOSEL); }
    bool isNoMerge()            const { return (ld_flags & CDL_NOMERGE); }
    bool isNoDRC()              const { return (ld_flags & CDL_NODRC); }
    bool isInvalid()            const { return (ld_flags & CDL_INVALID); }
    bool isTmpSkip()            const { return (ld_flags & CDL_SKIP); }

    bool isBlink()              const { return (ld_flags & CDL_BLINK); }
    bool isCut()                const { return (ld_flags & CDL_CUT); }
    bool isOutlined()           const { return (ld_flags & CDL_OUTLINED); }
    bool isOutlinedFat()        const { return (ld_flags & CDL_OUTLINEDFAT); }
    bool isFilled()             const { return (ld_flags & CDL_FILLED); }

    bool isConductor()          const { return (ld_flags & CDL_CONDUCTOR); }
    bool isRouting()            const { return (ld_flags & CDL_ROUTING); }
    bool isGroundPlane()        const { return (ld_flags & CDL_GROUNDPLANE); }
    bool isInContact()          const { return (ld_flags & CDL_IN_CONTACT); }
    bool isVia()                const { return (ld_flags & CDL_VIA); }
    bool isViaCut()             const { return (ld_flags & CDL_VIACUT); }
    bool isDielectric()         const { return (ld_flags & CDL_DIELECTRIC); }
    bool isPlanarizingSet()     const { return (ld_flags & CDL_PLANARIZE); }
    bool isDarkField()          const { return (ld_flags & CDL_DARKFIELD); }

    bool isElecStd()            const { return (ld_flags2 & CDLPRV_ELECSTD); }

    // Set flags.
    void setWireActive(bool b) { ld_flags = b ?
                (ld_flags | CDL_WIRE) : (ld_flags & ~CDL_WIRE); }
    void setImmutable(bool b) { ld_flags = b ?
                (ld_flags | CDL_IMMUT) : (ld_flags & ~CDL_IMMUT); }
    void setNoInstView(bool b) { ld_flags = b ?
                (ld_flags | CDL_NOINSTVW) : (ld_flags & ~CDL_NOINSTVW); }
    void setSymbolic(bool b) { ld_flags = b ?
                (ld_flags | CDL_SYMBOLIC) : (ld_flags & ~CDL_SYMBOLIC); }
    void setInvisible(bool b) { ld_flags = b ?
                (ld_flags | CDL_INVIS) : (ld_flags & ~CDL_INVIS); }
    void setRstInvisible(bool b) { ld_flags = b ?
                (ld_flags | CDL_RSTINVIS) : (ld_flags & ~CDL_RSTINVIS); }
    void setNoSelect(bool b) { ld_flags = b ?
                (ld_flags | CDL_NOSELECT) : (ld_flags & ~CDL_NOSELECT); }
    void setRstNoSelect(bool b) { ld_flags = b ?
                (ld_flags | CDL_RSTNOSEL) : (ld_flags & ~CDL_RSTNOSEL); }
    void setNoMerge(bool b) { ld_flags = b ?
                (ld_flags | CDL_NOMERGE) : (ld_flags & ~CDL_NOMERGE); }
    void setNoDRC(bool b) { ld_flags = b ?
                (ld_flags | CDL_NODRC) : (ld_flags & ~CDL_NODRC); }
    void setInvalid(bool b) { ld_flags = b ?
                (ld_flags | CDL_INVALID) : (ld_flags & ~CDL_INVALID); }
    void setTmpSkip(bool b) { ld_flags = b ?
                (ld_flags | CDL_SKIP) : (ld_flags & ~CDL_SKIP); }

    void setBlink(bool b) { ld_flags = b ?
                (ld_flags | CDL_BLINK) : (ld_flags & ~CDL_BLINK); }
    void setCut(bool b) { ld_flags = b ?
                (ld_flags | CDL_CUT) : (ld_flags & ~CDL_CUT); }
    void setOutlined(bool b) { ld_flags = b ?
                (ld_flags | CDL_OUTLINED) : (ld_flags & ~CDL_OUTLINED); }
    void setOutlinedFat(bool b) { ld_flags = b ?
                (ld_flags | CDL_OUTLINEDFAT) : (ld_flags & ~CDL_OUTLINEDFAT); }
    void setFilled(bool b) { ld_flags = b ?
                (ld_flags | CDL_FILLED) : (ld_flags & ~CDL_FILLED); }

    void setConductor(bool b) { ld_flags = b ?
                (ld_flags | CDL_CONDUCTOR) : (ld_flags & ~CDL_CONDUCTOR); }
    void setRouting(bool b) { ld_flags = b ?
                (ld_flags | CDL_ROUTING) : (ld_flags & ~CDL_ROUTING); }
    void setGroundPlane(bool b) { ld_flags = b ?
                (ld_flags | CDL_GROUNDPLANE) : (ld_flags & ~CDL_GROUNDPLANE); }
    void setInContact(bool b) { ld_flags = b ?
                (ld_flags | CDL_IN_CONTACT) : (ld_flags & ~CDL_IN_CONTACT); }
    void setVia(bool b) { ld_flags = b ?
                (ld_flags | CDL_VIA) : (ld_flags & ~CDL_VIA); }
    void setViaCut(bool b) { ld_flags = b ?
                (ld_flags | CDL_VIACUT) : (ld_flags & ~CDL_VIACUT); }
    void setDielectric(bool b) { ld_flags = b ?
                (ld_flags | CDL_DIELECTRIC) : (ld_flags & ~CDL_DIELECTRIC); }
    void setPlanarizingSet(bool b) { ld_flags = b ?
                (ld_flags | CDL_PLANARIZE) : (ld_flags & ~CDL_PLANARIZE); }
    void setDarkField(bool b) { ld_flags = b ?
                (ld_flags | CDL_DARKFIELD) : (ld_flags & ~CDL_DARKFIELD); }

    void setElecStd(bool b) { ld_flags2 = b ?
                (ld_flags2 | CDLPRV_ELECSTD) : (ld_flags2 & ~CDLPRV_ELECSTD); }
    // End of flag-setting.

    // Planarization control, for extraction and cross-section
    // display.  Planarizing can be explicitly set, defaults to on for
    // vias and conductors.
    bool isPlanarizing() const
        {
            if (isPlanarizingSet())
                return (ld_flags2 & CDLPRV_PLZASSET);
            return (isVia() || isViaCut() || isConductor());
        }

    void setPlanarizing(bool set, bool plz)
        {
            setPlanarizingSet(set);
            ld_flags2 = plz ?
                (ld_flags2 | CDLPRV_PLZASSET) : (ld_flags2 & ~CDLPRV_PLZASSET);
        }

    unsigned int getAttrFlags()  const { return (ld_flags & CDL_ATTR_FLAGS); }
    void setAttrFlags(unsigned int f)
        {
            ld_flags = (ld_flags & ~CDL_ATTR_FLAGS) | (f & CDL_ATTR_FLAGS);
        }

    bool isSelectable() const
        {
            if (!ld_phys_index || !ld_elec_index)
                return (true);  // Cell layer is always selectable.
            return (!(isNoSelect() || isInvisible()));
        }

    unsigned int oaLayerNum()   const { return (ld_oa_layernum); }
    unsigned int oaPurposeNum() const { return (ld_oa_purpose); }

    // This is the "id name" in the form "layer[:purpose]".
    const char *name()          const { return (ld_idname); };

    // Optional alias name.
    const char *lppName()       const { return (ld_lpp_name); }

    const char *description()   const { return (ld_description); }
    void setDescription(const char *d)
        {
            char *s = lstring::copy(d);
            delete [] ld_description;
            ld_description = s;
        }

    const char *drvExpr()       const { return (ld_drv_expr); }
    void setDrvExpr(const char *d)
        {
            char *s = lstring::copy(d);
            delete [] ld_drv_expr;
            ld_drv_expr = s;
        }

    unsigned int drvMode()      const { return (ld_drv_mode); }
    void setDrvMode(int i)            { ld_drv_mode = i & 3; }
    unsigned int drvIndex()     const { return (ld_drv_index); }
    void setDrvIndex(int i)           { ld_drv_index = i; }

    // Derived layers are not in the table, so we have to force the
    // name setting.
    void initName(const char *n)
        {
            if (layerType() == CDLderived) {
                char *s = lstring::copy(n);
                delete [] ld_idname;
                ld_idname = s;
            }
        }

    unsigned int datatype(int i) const
        {
            if (i < 0 || i > 3)
                return (0);
            return (ld_datatypes[i]);
        }
    void setDatatype(int i, unsigned int dt)
        {
            if (i >= 0 && i < 4)
                ld_datatypes[i] = dt;
        }

    void *dspData()             const { return (ld_dsp_data); }
    void setDspData(void *d)          { ld_dsp_data = d; }
    void *appData()             const { return (ld_app_data); }
    void setAppData(void *d)          { ld_app_data = d; }

    strm_odata *strmOut()       const { return (ld_strm_out); }
    void setStrmOut(strm_odata *o)    { ld_strm_out = o; }
    strm_idata *strmIn()        const { return (ld_strm_in); }
    void setStrmIn(strm_idata *i)     { ld_strm_in = i; }

    int physIndex()             const { return (ld_phys_index); }
    int elecIndex()             const { return (ld_elec_index); }
    int index(DisplayMode m)
        const { return (m == Physical ? physIndex() : elecIndex()); }

private:
    // These are called from friend cCDldb.
    void setPhysIndex(int i)       { ld_phys_index = i; }
    void setElecIndex(int i)       { ld_elec_index = i; }
    void setIndex(DisplayMode m, int i)
        {
            if (m == Physical)
                ld_phys_index = i;
            else 
                ld_elec_index = i;
        }
    void setFlags(unsigned int f)  { ld_flags = f; }
    void setOaLpp(unsigned int lnum, unsigned int pnum)
        {
            ld_oa_layernum = lnum;
            ld_oa_purpose = pnum;
        }
    void setLPPname(const char *lppnm)
        {
            char *s = lstring::copy(lppnm);
            delete [] ld_lpp_name;
            ld_lpp_name = s;
        }
    void setIdName();

    unsigned int ld_flags;          // flags
    unsigned char ld_flags2;        // flags for internal/misc. use.
    unsigned char ld_drv_mode;      // join/split for derived layers
    unsigned short ld_drv_index;    // layer number for derived layers

    // We no longer make a distinction between layers used in physical
    // and electrical mode, layers can appear in both.

    int ld_phys_index;              // index in physical table, -1 if absent
    int ld_elec_index;              // index in electrical table, -1 if absent
// datatypes
//
#define CDNODRC_DT     0
#define CDSPECIAL_DT1  1
#define CDSPECIAL_DT2  2
#define CDSPECIAL_DT3  3

    unsigned short ld_datatypes[4]; // special gds datatypes

    unsigned int ld_oa_layernum;    // OA layer number
    unsigned int ld_oa_purpose;     // OA purpose number

    strm_idata *ld_strm_in;         // linked list of input layers
    strm_odata *ld_strm_out;        // output data
    char *ld_idname;                // saved idname (layer[:purpose])
    char *ld_lpp_name;              // optional alias name
    char *ld_description;           // optional description
    char *ld_drv_expr;              // derived layer expression string
    void *ld_dsp_data;              // hook for display attribute data
    void *ld_app_data;              // hook for application data
};


// Linked list element for CDl.
struct CDll
{
    CDll() { next = 0; ldesc = 0; }
    CDll(CDl *l, CDll *n) { next = n; ldesc = l; }

    static void destroy(CDll *l)
        {
            while (l) {
                CDll *lx = l;
                l = l->next;
                delete lx;
            }
        }

    static bool inlist(const CDll *thisl, CDl *ld)
        {
            for (const CDll *l = thisl; l; l = l->next) {
                if (l->ldesc == ld)
                    return (true);
            }
            return (false);
        }

    static CDll *unlink(CDll **list, CDl *which)
        {
            if (!list)
                return (0);
            CDll *p = 0;
            for (CDll *l = *list; l; p = l, l = l->next) {
                if (l->ldesc == which) {
                    if (!p)
                        *list = l->next;
                    else
                        p->next = l->next;
                    return (l);
                }
            }
            return (0);
        }

    CDll *next;
    CDl *ldesc;
};

#define LARY_INIT_SIZE 16
#define LARY_SIZE_INCR 16

// Layer array, corresponds to visible layer table.
//
struct CDlary
{
    CDlary(DisplayMode mode)
        {
            ary_layers = new CDl*[LARY_INIT_SIZE];
            ary_numused = 0;
            ary_allocated = LARY_INIT_SIZE;
            ary_mode = mode;
            memset(ary_layers, 0, ary_allocated*sizeof(CDl*));
        }

    ~CDlary()
        {
            delete [] ary_layers;
        }

    void clear()
        {
            for (int i = 0; i < ary_numused; i++) {
                ary_layers[i]->setIndex(ary_mode, -1);

                // If the layer is no longer in use, free it.
                if (ary_layers[i]->physIndex() == -1 &&
                        ary_layers[i]->elecIndex() == -1)
                    delete ary_layers[i];

                ary_layers[i] = 0;
            }
            ary_numused = 0;
        }

    CDl *layer(int i)
        {
            if (i >= 0 && i < ary_numused)
                return (ary_layers[i]);
             return (0);
        }

    void set_layer(int i, CDl *ld)
        {
            if (i >= 0 && i < ary_allocated)
                ary_layers[i] = ld;
        }

    void insert(CDl *lnew, int ix)
        {
            if (ary_numused + 1 >= ary_allocated) {
                CDl **tmp = new CDl*[ary_allocated + LARY_SIZE_INCR];
                for (int i = 0; i < ary_allocated; i++)
                    tmp[i] = ary_layers[i];
                for (int i = 0; i < LARY_SIZE_INCR; i++)
                    tmp[ary_allocated + i] = 0;
                delete [] ary_layers;
                ary_layers = tmp;
                ary_allocated += LARY_SIZE_INCR;
            }
            if (ix < 0 || ix > ary_numused)
                ix = ary_numused;
            for (int i = ary_numused; i > ix; i--) {
                ary_layers[i] = ary_layers[i-1];
                ary_layers[i]->setIndex(ary_mode, i);
            }
            lnew->setIdName();
            lnew->setIndex(ary_mode, ix);
            ary_layers[ix] = lnew;
            ary_numused++;
        }

    void remove(int ix)
        {
            if (ix < 0)
                return;
            for (int i = ix; i < ary_numused-1; i++) {
                ary_layers[i] = ary_layers[i+1];
                ary_layers[i]->setIndex(ary_mode, i);
            }
            ary_layers[ary_numused-1] = 0;
            ary_numused--;
        }

    int numused()               { return (ary_numused); }
    void set_numused(int n)     { ary_numused = n; }
    void inc_numused()          { ary_numused++; }
    int allocated()             { return (ary_allocated); }
    CDl **layers()              { return (ary_layers); }

private:
    CDl **ary_layers;
    int ary_numused;
    int ary_allocated;
    DisplayMode ary_mode;
};

#endif

