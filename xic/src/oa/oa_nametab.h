
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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

#ifndef OA_NAMETAB_H
#define OA_NAMETAB_H

//
// The name table, used when reading cells from OA to determine which
// cells have already been read, and performs name aliasing to avoid
// conflicts.  This is persistent and accumulative until explicitly
// cleared.
//

// Status flags used in the cellname table.
//
#define OAL_READP   0x1
#define OAL_READE   0x2
#define OAL_READS   0x4
#define OAL_NOOA    0x8
#define OAL_REFP    0x10
#define OAL_REFE    0x20
//
// OAL_READP: Have read maskLayer data for cell.
// OAL_READE: Have read schematic data for cell.
// OAL_READS: Have read symbolic data for cell.
// OAL_NOOA:  Cell was referenced, but not resolved in OA libraries.
// OAL_REFP:  Cell was referenced in layout data.
// OAL_REFE:  Cell was referenced in schematic data.

// Name of the Virtuoso analogLib.
#define ANALOG_LIB      "analogLib"

// Cells from the ANALOG_LIB will be given this prefix.
#define ANALOG_LIB_PFX  "alib_"

// The class provides two interfaces:  The find/update functions keep
// track of the Xic cell names created, with a status flag.  The
// getMasterName group takes care of cell name aliasing.  Aliasing is
// needed to avoid name clashes with Xic library devices, and in the
// situation where a design reads the same cell name from two
// different libraries.  It also generates pcell sub-master names.

class cOAnameTab
{
public:
    cOAnameTab();
    ~cOAnameTab();

    unsigned long findCname(const oaString &cname)
        {
            if (!nt_cname_tab)
                return (0);
            return (findCname(CD()->CellNameTableAdd(cname)));
        }

    void updateCname(const oaString &cname, unsigned long f)
        {
            updateCname(CD()->CellNameTableAdd(cname), f);
        }

    SymTab *cnameTab()          { return (nt_cname_tab); }

    unsigned long findCname(CDcellName);
    void updateCname(CDcellName, unsigned long);
    void clearCnameTab();

    CDcellName getMasterName(const oaScalarName&, const oaScalarName&,
        const oaScalarName&, const oaParamArray&, bool);
    CDcellName cellNameAlias(const oaScalarName&, const oaScalarName&, bool);
    CDcellName getNewName(CDcellName);
    stringlist *listLibNames();

private:
    SymTab *nt_cname_tab;       // Xic cellname -> flags
    SymTab *nt_libtab_tab;      // libname -> cellname table -> Xic cellname
};

#endif

