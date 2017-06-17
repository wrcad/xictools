
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
 $Id: view_menu.h,v 5.8 2008/02/22 04:31:28 stevew Exp $
 *========================================================================*/

#ifndef VIEWMENU_H
#define VIEWMENU_H

// View menu
enum
{
    viewMenu,
    viewMenuView,
    viewMenuSced,
    viewMenuPhys,
    viewMenuExpnd,
    viewMenuZoom,
    viewMenuVport,
    viewMenuPeek,
    viewMenuCsect,
    viewMenuRuler,
    viewMenuInfo,
    viewMenuAlloc,
    viewMenu_END
};

// Subwindow View Menu
enum
{
    subwViewMenu,
    subwViewMenuView,
    subwViewMenuSced,
    subwViewMenuPhys,
    subwViewMenuExpnd,
    subwViewMenuZoom,
    subwViewMenuWdump,
    subwViewMenuLshow,
    subwViewMenuSwap,
    subwViewMenuLoad,
    subwViewMenuCancl,
    subwViewMenu_END
};

#define    MenuVIEW      "view"
#define    MenuSCED      "sced"
#define    MenuPHYS      "phys"
#define    MenuEXPND     "expnd"
#define    MenuZOOM      "zoom"
#define    MenuVPORT     "vport"
#define    MenuPEEK      "peek"
#define    MenuCSECT     "csect"
#define    MenuRULER     "ruler"
#define    MenuINFO      "info"
#define    MenuALLOC     "alloc"
#define    MenuWDUMP     "wdump"
#define    MenuLSHOW     "lshow"
#define    MenuSWAP      "swap"
#define    MenuLOAD      "load"
#define    MenuCANCL     "cancl"

#endif

