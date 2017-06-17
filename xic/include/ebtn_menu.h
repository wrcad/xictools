
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
 $Id: ebtn_menu.h,v 5.2 2015/02/04 06:16:05 stevew Exp $
 *========================================================================*/

#ifndef SCED_MENU_H
#define SCED_MENU_H

// Button Menu (Electrical)
enum
{
    btnElecMenu,
    btnElecMenuXform,
    btnElecMenuPlace,
    btnElecMenuDevs,
    btnElecMenuShape,
    btnElecMenuWire,
    btnElecMenuLabel,
    btnElecMenuErase,
    btnElecMenuBreak,
    btnElecMenuSymbl,
    btnElecMenuNodmp,
    btnElecMenuSubct,
    btnElecMenuTerms,
    btnElecMenuSpCmd,
    btnElecMenuRun,
    btnElecMenuDeck,
    btnElecMenuPlot,
    btnElecMenuIplot,
    btnElecMenu_END
};

// Ordering for the shapes pop-up menu.
enum ShapeType
    { ShBox, ShPoly, ShArc, ShDot, ShTri, ShTtri, ShAnd, ShOr, ShSides };

#define    MenuDEVS      "devs"
#define    MenuSHAPE     "shape"
#define    MenuWIRE      "wire"
#define    MenuLABEL     "label"
#define    MenuERASE     "erase"
#define    MenuBREAK     "break"
#define    MenuSYMBL     "symbl"
#define    MenuNODMP     "nodmp"
#define    MenuSUBCT     "subct"
#define    MenuTERMS     "terms"
#define    MenuSPCMD     "spcmd"
#define    MenuRUN       "run"
#define    MenuDECK      "deck"
#define    MenuPLOT      "plot"
#define    MenuIPLOT     "iplot"
#define    MenuMUTUL     "mutul"

extern MenuFunc  M_ShowMutual;;

#endif

