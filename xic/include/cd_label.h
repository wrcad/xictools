
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
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
 $Id: cd_label.h,v 5.8 2016/02/21 18:49:43 stevew Exp $
 *========================================================================*/

#ifndef CD_LABEL_H
#define CD_LABEL_H

//-------------------------------------------------------------------------
// Primitive label type
//-------------------------------------------------------------------------

struct hyList;

// label element
struct Label
{
    Label()
        {
            label = 0;
            x = y = 0;
            width = height = 0;
            xform = 0;
        }

    Label(hyList *str, int xx, int yy, int w, int h, int xf)
        {
            label = str;
            x = xx; y = yy;
            width = w;
            height = h;
            xform = xf;
        }

    void computeBB(BBox*);
    bool intersect(const BBox*, bool);

    static void TransformLabelBB(int, BBox*, Point**);
    static void InvTransformLabelBB(int, BBox*, Point*);

    hyList *label;
    int x, y, width, height;
    int xform;
};

// Code for xform field, see graphics.h
// bits  action
// 0-1   0-no rotation, 1-90, 2-180, 3-270
// 2     mirror y after rotation
// 3     mirror x after rotation and mirror y
// 4     shift rotation to 45, 135, 225, 315
// 5-6   horiz justification 00,11 left, 01 center, 10 right
// 7-8   vert justification 00,11 bottom, 01 center, 10 top
// 9-10  font number

#endif

