
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
 $Id: ext_menu.h,v 5.15 2014/07/28 04:44:01 stevew Exp $
 *========================================================================*/

#ifndef EXTMENU_H
#define EXTMENU_H

// Extract Menu
enum
{
    extMenu,
    extMenuExcfg,
    extMenuSel,
    extMenuDvsel,
    extMenuSourc,
    extMenuExset,
    extMenuPnet,
    extMenuEnet,
    extMenuLvs,
    extMenuExC,
    extMenuExLR,
    extMenu_END
};

#define    MenuEXTRC     "extrc"
#define    MenuEXCFG     "excfg"
#define    MenuEXSEL     "exsel"
#define    MenuDVSEL     "dvsel"
#define    MenuSOURC     "sourc"
#define    MenuEXSET     "exset"
#define    MenuPNET      "pnet"
#define    MenuENET      "enet"
#define    MenuLVS       "lvs"
#define    MenuEXC       "exc"
#define    MenuEXLR      "exlr"

#endif

