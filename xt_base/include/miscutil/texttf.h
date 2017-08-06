
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef TEXTTF_H
#define TEXTTF_H

// Presentation code bits for text strings.
//
// bits  action
// 0-1   0-no rotation, 1-90, 2-180, 3-270.
// 2     mirror y after rotation
// 3     mirror x after rotation and mirror y
// 4     shift rotation to 45, 135, 225, 315
// 5-6   horiz justification 00 left, 01 center, 10,11 right
// 7-8   vert justification 00 bottom, 01 center, 10,11 top
// 9-10  font number
// 11    unused, reserved
// 12    show text
// 13    hide text
// 14    text is shown only when container is top-level
// 15    limit lines displayed to an app-set value.
//
// The show/hide bits are for implementing a clickable text display,
// where the label text can be shown or "hidden" by rendering a small
// glyph instead.  At most one of these bits should be set.  Either
// bit overrides the default which is in force when neither is set.
//
#define TXTF_ROT    0x3
#define TXTF_MY     0x4
#define TXTF_MX     0x8
#define TXTF_45     0x10
#define TXTF_XF     0x1f
#define TXTF_HJC    0x20
#define TXTF_HJR    0x40
#define TXTF_VJC    0x80
#define TXTF_VJT    0x100
#define TXTF_FNT    0x600
#define TXTF_SHOW   0x1000
#define TXTF_HIDE   0x2000
#define TXTF_TLEV   0x4000
#define TXTF_LIML   0x8000

// Return the font index (0-3).
//
#define TXTF_FONT_INDEX(xf) (((xf) & TXTF_FNT) >> 9)

// Set the font index field.
//
#define TXTF_SET_FONT(xf, ft)  (xf |= (((ft) & 0x3) << 9))

// Text rep. utilities in texttf.cc.
extern char *xform_to_string(unsigned int);
extern unsigned int string_to_xform(const char*);

#endif

