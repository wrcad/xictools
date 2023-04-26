
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

#ifndef ATTR_MENU_H
#define ATTR_MENU_H

// Attributes Menu
enum
{
    attrMenu,
    attrMenuUpdat,
    attrMenuKeymp,
    attrMenuMacro,
    attrMenuMainWin,
    attrMenuAttr,
    attrMenuDots,
    attrMenuFont,
    attrMenuColor,
    attrMenuFill,
    attrMenuEdlyr,
    attrMenuLpedt,
    attrMenu_END
};

// Subwindow Attributes Menu
enum
{
    subwAttrMenu,
    subwAttrMenuFreez,
    subwAttrMenuCntxt,
    subwAttrMenuProps,
    subwAttrMenuLabls,
    subwAttrMenuLarot,
    subwAttrMenuCnams,
    subwAttrMenuCnrot,
    subwAttrMenuNouxp,
    subwAttrMenuObjs,
    subwAttrMenuTinyb,
    subwAttrMenuNosym,
    subwAttrMenuGrid,
    subwAttrMenu_END
};

enum
{
    objSubMenu,
    objSubMenuBoxes,
    objSubMenuPolys,
    objSubMenuWires,
    objSubMenuLabels,
    objSubMenu_END
};

#define    MenuUPDAT    "updat"
#define    MenuKEYMP    "keymp"
#define    MenuMACRO    "macro"
#define    MenuMAINW    "mainw"
#define    MenuATTR     "attr"
#define    MenuDOTS     "dots"
#define    MenuFONT     "font"
#define    MenuCOLOR    "color"
#define    MenuFILL     "fill"
#define    MenuEDLYR    "edlyr"
#define    MenuLPEDT    "lpedt"

#define    MenuFREEZ    "freez"
#define    MenuCNTXT    "cntxt"
#define    MenuPROPS    "props"
#define    MenuLABLS    "labls"
#define    MenuLAROT    "larot"
#define    MenuCNAMS    "cnams"
#define    MenuCNROT    "cnrot"
#define    MenuNOUXP    "nouxp"
#define    MenuOBJS     "objs"
#define    MenuTINYB    "tinyb"
#define    MenuNOSYM    "nosym"
#define    MenuGRID     "grid"

#define    MenuDPYB     "dpyb"
#define    MenuDPYP     "dpyp"
#define    MenuDPYW     "dpyw"
#define    MenuDPYL     "dpyl"

#endif

