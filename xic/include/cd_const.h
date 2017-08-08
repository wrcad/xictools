
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

#ifndef CD_CONST_H
#define CD_CONST_H

#include "miscutil/miscmath.h"
#include <sys/types.h>
#include <stdint.h>   // for int64_t, uint64_t


//-------------------------------------------------------------------------
// Global symbols and types

struct BBox;

#ifdef NEED_INT64_TYPEDEF
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif

// Data type used for object identification from various allocators
// and hash tables.
typedef unsigned int ticket_t;
#define NULL_TICKET (ticket_t)(-1)

// Passeed as a CDc argument to various functions, forces the top-level
// CDs to ignore symbolic representation in electrical mode.
#define CD_NO_SYMBOLIC (CDc*)(-1)

// File formats and origins.
enum
{
    Fnone,      // Cell created in memory of otherwsie undetermined  type.
    Fnative,    // From native cell file.
    Fgds,       // From GDSII file.
    Fcgx,       // From CGX file.
    Foas,       // From OASIS file.
    Fcif,       // From CIF file.
    Foa         // From OpenAccess library.
};
typedef unsigned char FileType;

// Database resolution.  This was previously fixed at 1000 units per
// micron for both electrical and physical modes.  Now, we allow the
// physical resolution to be adjusted, however the electrical
// resolution must remain fixed at 1000, since internal units are used
// directly in properties in existing saved files.
//
extern int CDphysResolution;
extern const int CDelecResolution;

// Database extent.
extern const int CDinfinity;        // 1e9
extern const BBox CDinfiniteBB;     // -1e9,-1e9 -- 1e9,1e9
extern const BBox CDnullBB;         // "no area"

// Default label character size (microns).  In physical mode, this can
// be modified from within the application.
//
#define CD_DEF_TEXT_HEI 1.0
#define CD_MIN_TEXT_HEI 0.01
#define CD_MAX_TEXT_HEI 10.0
#define CD_TEXT_WID_PER_HEI 0.6
extern double CDphysDefTextHeight;
extern double CDphysDefTextWidth;
extern const double CDelecDefTextHeight;
extern const double CDelecDefTextWidth;

// Types of objects.  These must be lowe-case letters due to the
// subclassing of RTelem.
#define CDINSTANCE          'c'
#define CDPOLYGON           'p'
#define CDLABEL             'l'
#define CDWIRE              'w'
#define CDBOX               'b'

// Types of transformations.
#define CDMIRRORX           'x'     // mirror in the direction of x
#define CDMIRRORY           'y'     // mirror in the direction of y
#define CDROTATE            'r'     // rotate by vector X,Y
#define CDTRANSLATE         't'     // translate to X,Y

// Copy/Move enum.
enum CDmcType { CDcopy, CDmove };

// Range of scale factors for conversion.
#define CDSCALEMIN          0.001
#define CDSCALEMAX          1000.0

// Range of magnification values supported.
#define CDMAGMIN            0.001
#define CDMAGMAX            1000.0

// Return status values and args to sCD::Error.
enum CDerrType
{
    CDfailed,               // unspecified failure
    CDok,                   // success
    CDbadLayer,             // no or bad layer given
    CDbadBox,               // zero width or length box
    CDbadPolygon,           // degenerate polygon
    CDbadWire,              // negative width or too few points
    CDbadLabel              // bad label
};

// This specifies how to open new instances, used in CDs::makeCall().
//
enum CDcallType { CDcallNone, CDcallDb };
//  CDcallNone              No special constraints.
//  CDcallDb                We are reading input and adding new cells
//                          to the database.

//-------------------------------------------------------------------------
// Macros and inlines

// Internal dimension to microns.
inline double MICRONS(int x) { return (((double)x)/CDphysResolution); }
inline double ELEC_MICRONS(int x) { return (((double)x)/CDelecResolution); }

// Microns to internal dimension.
inline int INTERNAL_UNITS(double d) { return (mmRnd(d*CDphysResolution)); }
inline int ELEC_INTERNAL_UNITS(double d) { return (mmRnd(d*CDelecResolution)); }

#endif

