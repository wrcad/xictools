
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
 $Id: cvrt_menu.h,v 5.11 2016/02/27 21:04:20 stevew Exp $
 *========================================================================*/

#ifndef CVRTMENU_H
#define CVRTMENU_H

// Convert Menu
enum
{
    cvrtMenu,
    cvrtMenuExprt,
    cvrtMenuImprt,
    cvrtMenuConvt,
    cvrtMenuAssem,
    cvrtMenuDiff,
    cvrtMenuCut,
    cvrtMenuTxted,
    cvrtMenu_END
};

#define    MenuCNVRT     "cnvrt"
#define    MenuEXPRT     "exprt"
#define    MenuIMPRT     "imprt"
#define    MenuCONVT     "convt"
#define    MenuASSEM     "assem"
#define    MenuDIFF      "diff"
#define    MenuCUT       "cut"
#define    MenuTXTED     "txted"

#endif

