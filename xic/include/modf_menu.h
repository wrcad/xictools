
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
 $Id: modf_menu.h,v 5.3 2012/09/02 18:52:25 stevew Exp $
 *========================================================================*/

#ifndef MODIFY_MENU_H
#define MODIFY_MENU_H

// MODIFY Menu
enum
{
    modfMenu,
    modfMenuUndo,
    modfMenuRedo,
    modfMenuDelet,
    modfMenuEundr,
    modfMenuMove,
    modfMenuCopy,
    modfMenuStrch,
    modfMenuChlyr,
    modfMenuMClcg,
    modfMenu_END
};

#define    MenuUNDO      "undo"
#define    MenuREDO      "redo"
#define    MenuDELET     "delet"
#define    MenuEUNDR     "eundr"
#define    MenuMOVE      "move"
#define    MenuCOPY      "copy"
#define    MenuSTRCH     "strch"
#define    MenuCHLYR     "chlyr"
#define    MenuMCLCG     "mclcg"

#endif

