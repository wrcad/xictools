
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
 $Id: help_menu.h,v 5.7 2013/05/29 20:31:50 stevew Exp $
 *========================================================================*/

#ifndef HELPMWNU_H
#define HELPMWNU_H

// Help Menu
enum
{
    helpMenu,
    helpMenuHelp,
    helpMenuMultw,
    helpMenuAbout,
    helpMenuNotes,
    helpMenuLogs,
    helpMenuDebug,
    helpMenu_END
};

// Subwindow Help Menu
enum
{
    subwHelpMenu,
    subwHelpMenuHelp,
    subwHelpMenu_END
};

#define    MenuHELP         "help"
#define    MenuMULTW        "multw"
#define    MenuABOUT        "about"
#define    MenuNOTES        "notes"
#define    MenuLOGS         "logs"
#define    MenuDBLOG        "dblog"

#endif

