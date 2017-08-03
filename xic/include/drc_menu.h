
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef DRCMENU_H
#define DRCMENU_H

// DRC Menu
enum
{
    drcMenu,
    drcMenuLimit,
    drcMenuSflag,
    drcMenuIntr,
    drcMenuNopop,
    drcMenuCheck,
    drcMenuPoint,
    drcMenuClear,
    drcMenuQuery,
    drcMenuErdmp,
    drcMenuErupd,
    drcMenuNext,
    drcMenuErlyr,
    drcMenuDredt,
    drcMenu_END
};

#define    MenuDRC       "drc"
#define    MenuLIMIT     "limit"
#define    MenuSFLAG     "sflag"
#define    MenuINTR      "intr"
#define    MenuNOPOP     "nopop"
#define    MenuCHECK     "check"
#define    MenuPOINT     "point"
#define    MenuCLEAR     "clear"
#define    MenuQUERY     "query"
#define    MenuERDMP     "erdmp"
#define    MenuERUPD     "erupd"
#define    MenuNEXT      "next"
#define    MenuERLYR     "erlyr"
#define    MenuDREDT     "dredt"

#endif

