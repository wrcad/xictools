
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: texttf.h,v 1.7 2013/02/13 00:18:28 stevew Exp $
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

