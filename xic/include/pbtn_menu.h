
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
 $Id: pbtn_menu.h,v 5.2 2015/02/04 06:16:05 stevew Exp $
 *========================================================================*/

#ifndef PSDMENU_H
#define PSDMENU_H

// Button Menu (Physical)
enum
{
    btnPhysMenu,
    btnPhysMenuXform,
    btnPhysMenuPlace,
    btnPhysMenuLabel,
    btnPhysMenuLogo,
    btnPhysMenuBox,
    btnPhysMenuPolyg,
    btnPhysMenuWire,
    btnPhysMenuStyle,
    btnPhysMenuRound,
    btnPhysMenuDonut,
    btnPhysMenuArc,
    btnPhysMenuSides,
    btnPhysMenuXor,
    btnPhysMenuBreak,
    btnPhysMenuErase,
    btnPhysMenuPut,
    btnPhysMenuSpin,
    btnPhysMenu_END
};

#define    MenuXFORM     "xform"
#define    MenuPLACE     "place"
#define    MenuLABEL     "label"
#define    MenuLOGO      "logo"
#define    MenuBOX       "box"
#define    MenuPOLYG     "polyg"
#define    MenuWIRE      "wire"
#define    MenuSTYLE     "style"
#define    MenuROUND     "round"
#define    MenuDONUT     "donut"
#define    MenuARC       "arc"
#define    MenuSIDES     "sides"
#define    MenuXOR       "xor"
#define    MenuBREAK     "break"
#define    MenuERASE     "erase"
#define    MenuPUT       "put"
#define    MenuSPIN      "spin"

#endif

