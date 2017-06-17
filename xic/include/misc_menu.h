
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
 $Id: misc_menu.h,v 5.12 2015/11/11 19:59:07 stevew Exp $
 *========================================================================*/

#ifndef MISCMENU_H
#define MISCMENU_H

// Misc. callbacks for UI objects
enum
{
    miscMenu,
    miscMenuMail,       // Email client
    miscMenuLtvis,      // Layer table visibility
    miscMenuLpal,       // Layer palette
    miscMenuSetcl,      // Set current layer from clicked-on object
    miscMenuSelcp,      // Selections control panel
    miscMenuDesel,      // Deselect all
    miscMenuRdraw,      // Redraw drawing windows
    miscMenu_END
};

#define    MenuMAIL      "mail"
#define    MenuLTVIS     "ltvis"
#define    MenuLPAL      "lpal"
#define    MenuSETCL     "setcl"
#define    MenuSELCP     "selcp"
#define    MenuDESEL     "desel"
#define    MenuRDRAW     "rdraw"

#endif

