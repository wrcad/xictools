
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

#ifndef SCRKIT_H
#define SCRKIT_H

#include "miscmath.h"

//
// Some misc. definitions from Xic.
//

// The physical and electrical resolutions.  Electrical is always 1000.
// Physical defaults to 1000, but can be changed.  Best not to change
// this in user code.
//
extern int CDphysResolution;
extern const int CDelecResolution;

// Internal dimension to microns.
inline double MICRONS(int x) { return (((double)x)/CDphysResolution); }
inline double ELEC_MICRONS(int x) { return (((double)x)/CDelecResolution); }

// Microns to internal dimension.
inline int INTERNAL_UNITS(double d) { return (mmRnd(d*CDphysResolution)); }
inline int ELEC_INTERNAL_UNITS(double d) { return (mmRnd(d*CDelecResolution)); }

// Maximum allowed call hierarchy depth.
#define CDMAXCALLDEPTH      40

// Physical or electrical mode.
enum DisplayMode { Physical=0, Electrical };

#define NO_EXTERNAL

#include "si_if_variable.h"
#include "si_args.h"
#include "si_scrfunc.h"

#endif

