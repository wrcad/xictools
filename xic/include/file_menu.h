
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
 $Id: file_menu.h,v 5.12 2015/03/29 03:31:52 stevew Exp $
 *========================================================================*/

#ifndef FILE_MENU_H
#define FILE_MENU_H

// File Menu
enum
{
    fileMenu,
    fileMenuFsel,
    fileMenuOpen,
    fileMenuSave,
    fileMenuSaveAs,
    fileMenuSaveAsDev,
    fileMenuHcopy,
    fileMenuFiles,
    fileMenuHier,
    fileMenuGeom,
    fileMenuLibs,
    fileMenuOAlib,
    fileMenuExit,
    fileMenu_END
};

#define    MenuFILE      "file"
#define    MenuFSEL      "fsel"
#define    MenuOPEN      "open"
#define    MenuSV        "sv"
#define    MenuSAVE      "save"
#define    MenuSADEV     "sadev"
#define    MenuHCOPY     "hcopy"
#define    MenuFILES     "files"
#define    MenuHIER      "hier"
#define    MenuGEOM      "geom"
#define    MenuLIBS      "libs"
#define    MenuOALIB     "oalib"
#define    MenuEXIT      "quit"

#endif

