
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

#ifndef DSP_IMAGE_H
#define DSP_IMAGE_H

#include "fio.h"
#include "fio_chd.h"
#include "fio_chd_flat.h"
#include "fio_cvt_base.h"
#include "raster.h"


//
// Definitions for in-memory image creation.
//

namespace ginterf {
    struct RGBzimg;
}

// This is a helper class passed to the function which renders
// unexpanded subcells and "tiny" boxes when composing an image from a
// CHD.
//
struct bnd_draw_t
{
    bnd_draw_t(WindowDesc *wd, cCHD *c, const BBox *bb)
    {
        bd_ntab = c->nameTab(wd->Mode());
        bd_AOI = bb;
        bd_numgeom = 0;
        bd_show_text = wd->Attrib()->label_instances(wd->Mode());
        bd_show_tiny = wd->Attrib()->show_tiny_bb(wd->Mode());
        bd_abort = false;
    }

    nametab_t *ntab()   { return (bd_ntab); }
    const BBox *AOI()   { return (bd_AOI); }
    int numgeom()       { return (bd_numgeom); }
    SLtype show_text()  { return (bd_show_text); }
    bool show_tiny()    { return (bd_show_tiny); }
    bool aborted()      { return (bd_abort); }

    void new_geom()
        {
            bd_numgeom++;
            if (!(bd_numgeom & 0xff)) {
                // check every 256 objects for efficiency
                dspPkgIf()->CheckForInterrupt();
                if (DSP()->Interrupt()) {
                    DSP()->SetInterrupt(DSPinterNone);
                    bd_abort = true;
                }
            }
        }

private:
    nametab_t *bd_ntab;
    const BBox *bd_AOI;
    int bd_numgeom;
    SLtype bd_show_text;
    bool bd_show_tiny;
    bool bd_abort;
};

// Back-end for image generation from CHD, uses RGBzimg.
//
struct zimg_backend : public cv_backend
{
    struct zb_layer_t
    {
        zb_layer_t(const Layer *lyr)
            {
                lname = lyr ? lstring::copy(lyr->name) : 0;
                layer = lyr ? lyr->layer : -1;
                datatype = lyr ? lyr->datatype : -1;
                pixel = 0;
                flags = 0;
                fill = 0;
                index = -1;
            }

        ~zb_layer_t()
            {
                delete [] lname;
            }

        bool isInvisible()      { return (flags & CDL_INVIS); }

        char *lname;
        int layer;
        int datatype;
        unsigned int pixel;
        unsigned int flags;
        GRfillType *fill;
        int index;
    };

    zimg_backend(WindowDesc*, RGBzimg*);
    ~zimg_backend();

    bool queue_layer(const Layer*, bool*);
    bool write_box(const BBox*);
    bool write_poly(const Poly*);
    bool write_wire(const Wire*);
    bool write_text(const Text*);

    int numgeom(int ng)
        {
            zb_numgeom += ng;
            return (zb_numgeom);
        }

private:
    WindowDesc *zb_wdesc;
    RGBzimg *zb_rgbimg;
    zb_layer_t *zb_layer;
    SymTab *zb_ltab;
    int zb_numgeom;
    bool zb_allow_layer_mapping;
};

#endif

