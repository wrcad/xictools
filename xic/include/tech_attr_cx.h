
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

#ifndef TECH_ATTR_CX_H
#define TECH_ATTR_CX_H


//
// Structure to hold display parameters active when a print driver is
// selected, in hard-copy mode.
//

// Per-layer attributes.
//
struct sLayerAttr
{
    uintptr_t tab_key()                 { return ((uintptr_t)la_ldesc); }
    sLayerAttr *tab_next()              { return (la_next); }
    void set_tab_next(sLayerAttr *n)    { la_next = n; }
    sLayerAttr *tgen_next(bool)         { return (la_next); }

    void set(CDl *ld)
        {
            la_next = 0;
            la_ldesc = ld;
            la_r = 0;
            la_g = 0;
            la_b = 0;
            la_pixel = 0;
            la_flags = 0;
            memset(la_hpglfill, 0, 4);
            memset(la_miscfill, 0, 4);
        }

    void save()
        {
            DspLayerParams *lp = dsp_prm(la_ldesc);
            la_r = lp->red();
            la_g = lp->green();
            la_b = lp->blue();
            la_pixel = lp->pixel();
            la_flags = la_ldesc->getAttrFlags();

            unsigned char *map = lp->fill()->newBitmap();
            la_fill.newMap(lp->fill()->nX(), lp->fill()->nY(), map);
            delete [] map;
        }

    void restore() const
        {
            DspLayerParams *lp = dsp_prm(la_ldesc);
            lp->set_red(la_r);
            lp->set_green(la_g);
            lp->set_blue(la_b);
            lp->set_pixel(la_pixel);
            la_ldesc->setAttrFlags(la_flags);

            GRfillType *pf = lp->fill();
            unsigned char *map = la_fill.newBitmap();
            pf->newMap(la_fill.nX(), la_fill.nY(), map);
            delete [] map;
            DSPmainDraw(DefineFillpattern(pf))
        }

    bool isInvisible()          const { return (la_flags & CDL_INVIS); }
    bool isCut()                const { return (la_flags & CDL_CUT); }
    bool isOutlined()           const { return (la_flags & CDL_OUTLINED); }
    bool isOutlinedFat()        const { return (la_flags & CDL_OUTLINEDFAT); }
    bool isFilled()             const { return (la_flags & CDL_FILLED); }

    void setInvisible(bool f) { la_flags = f ?
                (la_flags | CDL_INVIS) : (la_flags & ~CDL_INVIS); }
    void setCut(bool f) { la_flags = f ?
                (la_flags | CDL_CUT) : (la_flags & ~CDL_CUT); }
    void setOutlined(bool f) { la_flags = f ?
                (la_flags | CDL_OUTLINED) : (la_flags & ~CDL_OUTLINED); }
    void setOutlinedFat(bool f) { la_flags = f ?
                (la_flags | CDL_OUTLINEDFAT) : (la_flags & ~CDL_OUTLINEDFAT); }
    void setFilled(bool f) { la_flags = f ?
                (la_flags | CDL_FILLED) : (la_flags & ~CDL_FILLED); }

    int hpgl_fill(int i) const
        {
            return (i >= 0 && i < 4 ? la_hpglfill[i] : 0);
        }

    void set_hpgl_fill(int i, int c)
        {
            if (i >= 0 && i < 4)
                la_hpglfill[i] = c;
        }

    int misc_fill(int i) const
        {
            return (i >= 0 && i < 4 ? la_miscfill[i] : 0);
        }

    void set_misc_fill(int i, int c)
        {
            if (i >= 0 && i < 4)
                la_miscfill[i] = c;
        }

    void set_rgbp(int red, int grn, int blu, unsigned int pix)
        {
            la_r = red;
            la_g = grn;
            la_b = blu;
            la_pixel = pix;
        }

    int r()                     const { return (la_r); }
    int g()                     const { return (la_g); }
    int b()                     const { return (la_b); }

    GRfillType *fill()                { return (&la_fill); }
    const GRfillType *cfill()   const { return (&la_fill); }
    const CDl *ldesc()          const { return (la_ldesc); }

private:
    sLayerAttr *la_next;
    CDl *la_ldesc;

    int la_r, la_g, la_b;               // color values, 0-255
    unsigned int la_pixel;              // color encoding
    GRfillType la_fill;                 // fill spec

    unsigned int la_flags;              // layer desc flags
    unsigned char la_hpglfill[4];       // HP-GL fill spec
    unsigned char la_miscfill[4];       // other driver fill indices
};

// Main struct for attributes.
//
struct sAttrContext
{
    sAttrContext()
        {
            ac_layer_attrs = 0;
        }

    DSPattrib *attr()                   { return (&ac_attr); }

    sColorTab::sColorTabEnt *color(unsigned int i)
        {
            return (i < (unsigned int)AttrColorSize ? &ac_colors[i] : 0);
        }

    sLayerAttr *get_layer_attributes(CDl*, bool);
    void save_layer_attrs(CDl*);
    void restore_layer_attrs(CDl*);
    void dump(FILE*, CTPmode);

private:
    // For grid, etc.
    DSPattrib ac_attr;

    // Attribute colors.
    sColorTab::sColorTabEnt ac_colors[AttrColorSize];

    // Per-layer attributes.
    itable_t<sLayerAttr> *ac_layer_attrs;

    // Common allocator.
    static eltab_t<sLayerAttr> ac_allocator;
};

#endif

