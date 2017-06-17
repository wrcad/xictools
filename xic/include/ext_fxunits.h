
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.        *
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
 $Id: ext_fxunits.h,v 5.4 2014/07/05 04:35:03 stevew Exp $
 *========================================================================*/

#ifndef EXT_FXUNITS_H
#define EXT_FXUNITS_H

#include "cd_const.h"

//
// Units selection for the FastCap/FastHenry interface.
//


// Enum is indices into Units[] in ext_fxunits.cc.
enum e_unit { UNITS_M, UNITS_CM, UNITS_MM, UNITS_UM, UNITS_IN, UNITS_MIL };

// Default units: microns
#define FC_DEF_UNITS  UNITS_UM
#define FX_DEF_UNITS  UNITS_UM

// -- Units selection ---------------------------------------------------
//
// Assumes that techfile values of sigma/rho are MKS units
// 1/(ohm-m) and techfile thickness, lambda always microns.
//
struct unit_t
{
    unit_t(const char *n, const char *f, int ffw, double c, double l, double s)
        {
            u_name = n;
            u_float_format = f;
            u_ffwidth = ffw;
            u_coord_factor = c;
            u_lambda_factor = l;
            u_sigma_factor = s;
        }

    const char *name()              const { return (u_name); }
    const char *float_format()      const { return (u_float_format); }
    int field_width()               const { return (u_ffwidth); }
    double coord_factor() const
        { return (u_coord_factor*1e3/CDphysResolution); }
    double lambda_factor()          const { return (u_lambda_factor); }
    double sigma_factor()           const { return (u_sigma_factor); }

    static int find_unit(const char*);
    static const unit_t *units(e_unit);

private:
    const char *u_name;             // FastHenry code
    const char *u_float_format;     // printing format
    int u_ffwidth;                  // printing field width
    double u_coord_factor;          // factor to multiply internal units (nm)
    double u_lambda_factor;         // factor to multiply lambda (um))
    double u_sigma_factor;          // factor to multiply sigma (i/(ohm-m))
};

#endif

