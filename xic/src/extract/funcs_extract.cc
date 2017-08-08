
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

#include "config.h"
#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "kwstr_ext.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "ext_rlsolver.h"
#include "ext_pathfinder.h"
#include "sced.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_zgroup.h"
#include "si_parsenode.h"
// regex.h must come before si_handle.h to enable regex handle.
#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "libregex/regex.h"
#endif
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"
#include "layertab.h"
#include "tech_layer.h"
#include "miscutil/filestat.h"


////////////////////////////////////////////////////////////////////////
//
// Script Functions:  Extraction Functions
//
////////////////////////////////////////////////////////////////////////

namespace {
    namespace extract_funcs {
        // Menu Commands
        bool IFdumpPhysNetlist(Variable*, Variable*, void*);
        bool IFdumpElecNetlist(Variable*, Variable*, void*);
        bool IFsourceSpice(Variable*, Variable*, void*);
        bool IFextractAndSet(Variable*, Variable*, void*);
        bool IFfindPath(Variable*, Variable*, void*);
        bool IFfindPathOfGroup(Variable*, Variable*, void*);

        // Terminals
        bool IFlistTerminals(Variable*, Variable*, void*);
        bool IFfindTerminal(Variable*, Variable*, void*);
        bool IFcreateTerminal(Variable*, Variable*, void*);
        bool IFdestroyTerminal(Variable*, Variable*, void*);
        bool IFgetTerminalName(Variable*, Variable*, void*);
        bool IFsetTerminalName(Variable*, Variable*, void*);
        bool IFgetTerminalType(Variable*, Variable*, void*);
        bool IFsetTerminalType(Variable*, Variable*, void*);
        bool IFgetTerminalFlags(Variable*, Variable*, void*);
        bool IFsetTerminalFlags(Variable*, Variable*, void*);
        bool IFunsetTerminalFlags(Variable*, Variable*, void*);
        bool IFgetElecTerminalLoc(Variable*, Variable*, void*);
        bool IFsetElecTerminalLoc(Variable*, Variable*, void*);
        bool IFclearElecTerminalLoc(Variable*, Variable*, void*);

        // Physical Terminals
        bool IFlistPhysTerminals(Variable*, Variable*, void*);
        bool IFfindPhysTerminal(Variable*, Variable*, void*);
        bool IFcreatePhysTerminal(Variable*, Variable*, void*);
        bool IFhasPhysTerminal(Variable*, Variable*, void*);
        bool IFdestroyPhysTerminal(Variable*, Variable*, void*);
        bool IFgetPhysTerminalLoc(Variable*, Variable*, void*);
        bool IFsetPhysTerminalLoc(Variable*, Variable*, void*);
        bool IFgetPhysTerminalLayer(Variable*, Variable*, void*);
        bool IFsetPhysTerminalLayer(Variable*, Variable*, void*);
        bool IFgetPhysTerminalGroup(Variable*, Variable*, void*);
        bool IFgetPhysTerminalObject(Variable*, Variable*, void*);

        // Physical Conductor Groups
        bool IFgroup(Variable*, Variable*, void*);
        bool IFgetNumberGroups(Variable*, Variable*, void*);
        bool IFgetGroupBB(Variable*, Variable*, void*);
        bool IFgetGroupNode(Variable*, Variable*, void*);
        bool IFgetGroupName(Variable*, Variable*, void*);
        bool IFgetGroupNetName(Variable*, Variable*, void*);
        bool IFgetGroupCapacitance(Variable*, Variable*, void*);
        bool IFcountGroupObjects(Variable*, Variable*, void*);
        bool IFlistGroupObjects(Variable*, Variable*, void*);
        bool IFcountGroupVias(Variable*, Variable*, void*);
        bool IFlistGroupVias(Variable*, Variable*, void*);
        bool IFcountGroupDevContacts(Variable*, Variable*, void*);
        bool IFlistGroupDevContacts(Variable*, Variable*, void*);
        bool IFcountGroupSubcContacts(Variable*, Variable*, void*);
        bool IFlistGroupSubcContacts(Variable*, Variable*, void*);
        bool IFcountGroupTerminals(Variable*, Variable*, void*);
        bool IFlistGroupTerminals(Variable*, Variable*, void*);
        bool IFcountGroupPhysTerminals(Variable*, Variable*, void*);
        bool IFlistGroupPhysTerminals(Variable*, Variable*, void*);
        bool IFlistGroupTerminalNames(Variable*, Variable*, void*);

        // Physical Devices
        bool IFlistPhysDevs(Variable*, Variable*, void*);
        bool IFgetPdevName(Variable*, Variable*, void*);
        bool IFgetPdevIndex(Variable*, Variable*, void*);
        bool IFgetPdevDual(Variable*, Variable*, void*);
        bool IFgetPdevBB(Variable*, Variable*, void*);
        bool IFgetPdevMeasure(Variable*, Variable*, void*);
        bool IFlistPdevMeasures(Variable*, Variable*, void*);
        bool IFlistPdevContacts(Variable*, Variable*, void*);
        bool IFgetPdevContactName(Variable*, Variable*, void*);
        bool IFgetPdevContactBB(Variable*, Variable*, void*);
        bool IFgetPdevContactGroup(Variable*, Variable*, void*);
        bool IFgetPdevContactLayer(Variable*, Variable*, void*);
        bool IFgetPdevContactDev(Variable*, Variable*, void*);
        bool IFgetPdevContactDevName(Variable*, Variable*, void*);
        bool IFgetPdevContactDevIndex(Variable*, Variable*, void*);

        // Physical Subcircuits
        bool IFlistPhysSubckts(Variable*, Variable*, void*);
        bool IFgetPscName(Variable*, Variable*, void*);
        bool IFgetPscIndex(Variable*, Variable*, void*);
        bool IFgetPscIdNum(Variable*, Variable*, void*);
        bool IFgetPscInstName(Variable*, Variable*, void*);
        bool IFgetPscDual(Variable*, Variable*, void*);
        bool IFgetPscBB(Variable*, Variable*, void*);
        bool IFgetPscLoc(Variable*, Variable*, void*);
        bool IFgetPscTransform(Variable*, Variable*, void*);
        bool IFlistPscContacts(Variable*, Variable*, void*);
        bool IFisPscContactIgnorable(Variable*, Variable*, void*);
        bool IFgetPscContactName(Variable*, Variable*, void*);
        bool IFgetPscContactGroup(Variable*, Variable*, void*);
        bool IFgetPscContactSubcGroup(Variable*, Variable*, void*);
        bool IFgetPscContactSubc(Variable*, Variable*, void*);
        bool IFgetPscContactSubcName(Variable*, Variable*, void*);
        bool IFgetPscContactSubcIndex(Variable*, Variable*, void*);
        bool IFgetPscContactSubcIdNum(Variable*, Variable*, void*);
        bool IFgetPscContactSubcInstName(Variable*, Variable*, void*);

        // Electrical Devices
        bool IFlistElecDevs(Variable*, Variable*, void*);
        bool IFsetEdevProperty(Variable*, Variable*, void*);
        bool IFgetEdevProperty(Variable*, Variable*, void*);
        bool IFgetEdevObj(Variable*, Variable*, void*);

        // Resistance Extraction
        bool IFextractRL(Variable*, Variable*, void*);
        bool IFextractNetResistance(Variable*, Variable*, void*);
    }
    using namespace extract_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Menu Commands
    PY_FUNC(DumpPhysNetlist,        4,  IFdumpPhysNetlist);
    PY_FUNC(DumpElecNetlist,        4,  IFdumpElecNetlist);
    PY_FUNC(SourceSpice,            2,  IFsourceSpice);
    PY_FUNC(ExtractAndSet,          2,  IFextractAndSet);
    PY_FUNC(FindPath,               4,  IFfindPath);
    PY_FUNC(FindPathOfGroup,        2,  IFfindPathOfGroup);

    // Terminals
    PY_FUNC(ListTerminals,          0,  IFlistTerminals);
    PY_FUNC(FindTerminal,           8,  IFfindTerminal);
    PY_FUNC(CreateTerminal,         4,  IFcreateTerminal);
    PY_FUNC(DestroyTerminal,        1,  IFdestroyTerminal);
    PY_FUNC(GetTerminalName,        1,  IFgetTerminalName);
    PY_FUNC(SetTerminalName,        2,  IFsetTerminalName);
    PY_FUNC(GetTerminalType,        1,  IFgetTerminalType);
    PY_FUNC(SetTerminalType,        2,  IFsetTerminalType);
    PY_FUNC(GetTerminalFlags,       1,  IFgetTerminalFlags);
    PY_FUNC(SetTerminalFlags,       2,  IFsetTerminalFlags);
    PY_FUNC(UnsetTerminalFlags,     2,  IFunsetTerminalFlags);
    PY_FUNC(GetElecTerminalLoc,     3,  IFgetElecTerminalLoc);
    PY_FUNC(SetElecTerminalLoc,     3,  IFsetElecTerminalLoc);
    PY_FUNC(ClearElecTerminalLoc,   3,  IFclearElecTerminalLoc);

    // Physical Terminals
    PY_FUNC(ListPhysTerminals,      0,  IFlistPhysTerminals);
    PY_FUNC(FindPhysTerminal,       4,  IFfindPhysTerminal);
    PY_FUNC(CreatePhysTerminal,     4,  IFcreatePhysTerminal);
    PY_FUNC(HasPhysTerminal,        1,  IFhasPhysTerminal);
    PY_FUNC(DestroyPhysTerminal,    1,  IFdestroyPhysTerminal);
    PY_FUNC(GetPhysTerminalLoc,     2,  IFgetPhysTerminalLoc);
    PY_FUNC(SetPhysTerminalLoc,     3,  IFsetPhysTerminalLoc);
    PY_FUNC(GetPhysTerminalLayer,   1,  IFgetPhysTerminalLayer);
    PY_FUNC(SetPhysTerminalLayer,   2,  IFsetPhysTerminalLayer);
    PY_FUNC(GetPhysTerminalGroup,   1,  IFgetPhysTerminalGroup);
    PY_FUNC(GetPhysTerminalObject,  1,  IFgetPhysTerminalObject);

    // Physical Conductor Groups
    PY_FUNC(Group,                  0,  IFgroup);
    PY_FUNC(GetNumberGroups,        0,  IFgetNumberGroups);
    PY_FUNC(GetGroupBB,             2,  IFgetGroupBB);
    PY_FUNC(GetGroupNode,           1,  IFgetGroupNode);
    PY_FUNC(GetGroupName,           1,  IFgetGroupName);
    PY_FUNC(GetGroupNetName,        1,  IFgetGroupNetName);
    PY_FUNC(GetGroupCapacitance,    1,  IFgetGroupCapacitance);
    PY_FUNC(CountGroupObjects,      1,  IFcountGroupObjects);
    PY_FUNC(ListGroupObjects,       1,  IFlistGroupObjects);
    PY_FUNC(CountGroupVias,         1,  IFcountGroupVias);
    PY_FUNC(ListGroupVias,          1,  IFlistGroupVias);
    PY_FUNC(CountGroupDevContacts,  1,  IFcountGroupDevContacts);
    PY_FUNC(ListGroupDevContacts,   1,  IFlistGroupDevContacts);
    PY_FUNC(CountGroupSubcContacts, 1,  IFcountGroupSubcContacts);
    PY_FUNC(ListGroupSubcContacts,  1,  IFlistGroupSubcContacts);
    PY_FUNC(CountGroupTerminals,    1,  IFcountGroupTerminals);
    PY_FUNC(ListGroupTerminals,     1,  IFlistGroupTerminals);
    PY_FUNC(CountGroupPhysTerminals,1,  IFcountGroupPhysTerminals);
    PY_FUNC(ListGroupPhysTerminals, 1,  IFlistGroupPhysTerminals);
    PY_FUNC(ListGroupTerminalNames, 1,  IFlistGroupTerminalNames);

    // Physical Devices
    PY_FUNC(ListPhysDevs,           4,  IFlistPhysDevs);
    PY_FUNC(GetPdevName,            1,  IFgetPdevName);
    PY_FUNC(GetPdevIndex,           1,  IFgetPdevIndex);
    PY_FUNC(GetPdevDual,            1,  IFgetPdevDual);
    PY_FUNC(GetPdevBB,              2,  IFgetPdevBB);
    PY_FUNC(GetPdevMeasure,         2,  IFgetPdevMeasure);
    PY_FUNC(ListPdevMeasures,       1,  IFlistPdevMeasures);
    PY_FUNC(ListPdevContacts,       1,  IFlistPdevContacts);
    PY_FUNC(GetPdevContactName,     1,  IFgetPdevContactName);
    PY_FUNC(GetPdevContactBB,       2,  IFgetPdevContactBB);
    PY_FUNC(GetPdevContactGroup,    1,  IFgetPdevContactGroup);
    PY_FUNC(GetPdevContactLayer,    1,  IFgetPdevContactLayer);
    PY_FUNC(GetPdevContactDev,      1,  IFgetPdevContactDev);
    PY_FUNC(GetPdevContactDevName,  1,  IFgetPdevContactDevName);
    PY_FUNC(GetPdevContactDevIndex, 1,  IFgetPdevContactDevIndex);

    // Physical Subcircuits
    PY_FUNC(ListPhysSubckts,        6,  IFlistPhysSubckts);
    PY_FUNC(GetPscName,             1,  IFgetPscName);
    PY_FUNC(GetPscIndex,            1,  IFgetPscIndex);
    PY_FUNC(GetPscIdNum,            1,  IFgetPscIdNum);
    PY_FUNC(GetPscInstName,         1,  IFgetPscInstName);
    PY_FUNC(GetPscDual,             1,  IFgetPscDual);
    PY_FUNC(GetPscBB,               2,  IFgetPscBB);
    PY_FUNC(GetPscLoc,              2,  IFgetPscLoc);
    PY_FUNC(GetPscTransform,        3,  IFgetPscTransform);
    PY_FUNC(ListPscContacts,        1,  IFlistPscContacts);
    PY_FUNC(IsPscContactIgnorable,  1,  IFisPscContactIgnorable);
    PY_FUNC(GetPscContactName,      1,  IFgetPscContactName);
    PY_FUNC(GetPscContactGroup,     1,  IFgetPscContactGroup);
    PY_FUNC(GetPscContactSubcGroup, 1,  IFgetPscContactSubcGroup);
    PY_FUNC(GetPscContactSubc,      1,  IFgetPscContactSubc);
    PY_FUNC(GetPscContactSubcName,  1,  IFgetPscContactSubcName);
    PY_FUNC(GetPscContactSubcIndex, 1,  IFgetPscContactSubcIndex);
    PY_FUNC(GetPscContactSubcIdNum, 1,  IFgetPscContactSubcIdNum);
    PY_FUNC(GetPscContactSubcInstName, 1,  IFgetPscContactSubcInstName);

    // Electrical Devices
    PY_FUNC(ListElecDevs,           1,  IFlistElecDevs);
    PY_FUNC(SetEdevProperty,        3,  IFsetEdevProperty);
    PY_FUNC(GetEdevProperty,        2,  IFgetEdevProperty);
    PY_FUNC(GetEdevObj,             1,  IFgetEdevObj);

    // Resistance Extraction
    PY_FUNC(ExtractRL,        VARARGS,  IFextractRL );
    PY_FUNC(ExtractNetResistance, VARARGS, IFextractNetResistance);

    void py_register_extract()
    {
      // Menu Commands
      cPyIf::register_func("DumpPhysNetlist",        pyDumpPhysNetlist);
      cPyIf::register_func("DumpElecNetlist",        pyDumpElecNetlist);
      cPyIf::register_func("SourceSpice",            pySourceSpice);
      cPyIf::register_func("ExtractAndSet",          pyExtractAndSet);
      cPyIf::register_func("FindPath",               pyFindPath);
      cPyIf::register_func("FindPathOfGroup",        pyFindPathOfGroup);

      // Terminals
      cPyIf::register_func("ListTerminals",          pyListTerminals);
      cPyIf::register_func("FindTerminal",           pyFindTerminal);
      cPyIf::register_func("CreateTerminal",         pyCreateTerminal);
      cPyIf::register_func("DestroyTerminal",        pyDestroyTerminal);
      cPyIf::register_func("GetTerminalName",        pyGetTerminalName);
      cPyIf::register_func("SetTerminalName",        pySetTerminalName);
      cPyIf::register_func("GetTerminalType",        pyGetTerminalType);
      cPyIf::register_func("SetTerminalType",        pySetTerminalType);
      cPyIf::register_func("GetTerminalFlags",       pyGetTerminalFlags);
      cPyIf::register_func("SetTerminalFlags",       pySetTerminalFlags);
      cPyIf::register_func("UnsetTerminalFlags",     pyUnsetTerminalFlags);
      cPyIf::register_func("GetElecTerminalLoc",     pyGetElecTerminalLoc);
      cPyIf::register_func("SetElecTerminalLoc",     pySetElecTerminalLoc);
      cPyIf::register_func("ClearElecTerminalLoc",   pyClearElecTerminalLoc);

      // Physical Terminals
      cPyIf::register_func("ListPhysTerminals",      pyListPhysTerminals);
      cPyIf::register_func("FindPhysTerminal",       pyFindPhysTerminal);
      cPyIf::register_func("CreatePhysTerminal",     pyCreatePhysTerminal);
      cPyIf::register_func("HasPhysTerminal",        pyHasPhysTerminal);
      cPyIf::register_func("DestroyPhysTerminal",    pyDestroyPhysTerminal);
      cPyIf::register_func("GetPhysTerminalLoc",     pyGetPhysTerminalLoc);
      cPyIf::register_func("SetPhysTerminalLoc",     pySetPhysTerminalLoc);
      cPyIf::register_func("GetPhysTerminalLayer",   pyGetPhysTerminalLayer);
      cPyIf::register_func("SetPhysTerminalLayer",   pySetPhysTerminalLayer);
      cPyIf::register_func("GetPhysTerminalGroup",   pyGetPhysTerminalGroup);
      cPyIf::register_func("GetPhysTerminalObject",  pyGetPhysTerminalObject);

      // Physical Conductor Groups
      cPyIf::register_func("Group",                  pyGroup);
      cPyIf::register_func("GetNumberGroups",        pyGetNumberGroups);
      cPyIf::register_func("GetGroupBB",             pyGetGroupBB);
      cPyIf::register_func("GetGroupNode",           pyGetGroupNode);
      cPyIf::register_func("GetGroupName",           pyGetGroupName);
      cPyIf::register_func("GetGroupNetName",        pyGetGroupNetName);
      cPyIf::register_func("GetGroupCapacitance",    pyGetGroupCapacitance);
      cPyIf::register_func("CountGroupObjects",      pyCountGroupObjects);
      cPyIf::register_func("ListGroupObjects",       pyListGroupObjects);
      cPyIf::register_func("CountGroupVias",         pyCountGroupVias);
      cPyIf::register_func("ListGroupVias",          pyListGroupVias);
      cPyIf::register_func("CountGroupDevContacts",  pyCountGroupDevContacts);
      cPyIf::register_func("ListGroupDevContacts",   pyListGroupDevContacts);
      cPyIf::register_func("CountGroupSubcContacts", pyCountGroupSubcContacts);
      cPyIf::register_func("ListGroupSubcContacts",  pyListGroupSubcContacts);
      cPyIf::register_func("CountGroupTerminals",    pyCountGroupTerminals);
      cPyIf::register_func("ListGroupTerminals",     pyListGroupTerminals);
      cPyIf::register_func("CountGroupPhysTerminals",pyCountGroupPhysTerminals);
      cPyIf::register_func("ListGroupPhysTerminals", pyListGroupPhysTerminals);
      cPyIf::register_func("ListGroupTerminalNames", pyListGroupTerminalNames);

      // Physical Devices
      cPyIf::register_func("ListPhysDevs",           pyListPhysDevs);
      cPyIf::register_func("GetPdevName",            pyGetPdevName);
      cPyIf::register_func("GetPdevIndex",           pyGetPdevIndex);
      cPyIf::register_func("GetPdevDual",            pyGetPdevDual);
      cPyIf::register_func("GetPdevBB",              pyGetPdevBB);
      cPyIf::register_func("GetPdevMeasure",         pyGetPdevMeasure);
      cPyIf::register_func("ListPdevMeasures",       pyListPdevMeasures);
      cPyIf::register_func("ListPdevContacts",       pyListPdevContacts);
      cPyIf::register_func("GetPdevContactName",     pyGetPdevContactName);
      cPyIf::register_func("GetPdevContactBB",       pyGetPdevContactBB);
      cPyIf::register_func("GetPdevContactGroup",    pyGetPdevContactGroup);
      cPyIf::register_func("GetPdevContactLayer",    pyGetPdevContactLayer);
      cPyIf::register_func("GetPdevContactDev",      pyGetPdevContactDev);
      cPyIf::register_func("GetPdevContactDevName",  pyGetPdevContactDevName);
      cPyIf::register_func("GetPdevContactDevIndex", pyGetPdevContactDevIndex);

      // Physical Subcircuits
      cPyIf::register_func("ListPhysSubckts",        pyListPhysSubckts);
      cPyIf::register_func("GetPscName",             pyGetPscName);
      cPyIf::register_func("GetPscIndex",            pyGetPscIndex);
      cPyIf::register_func("GetPscIdNum",            pyGetPscIdNum);
      cPyIf::register_func("GetPscInstName",         pyGetPscInstName);
      cPyIf::register_func("GetPscDual",             pyGetPscDual);
      cPyIf::register_func("GetPscBB",               pyGetPscBB);
      cPyIf::register_func("GetPscLoc",              pyGetPscLoc);
      cPyIf::register_func("GetPscTransform",        pyGetPscTransform);
      cPyIf::register_func("ListPscContacts",        pyListPscContacts);
      cPyIf::register_func("IsPscContactIgnorable",  pyIsPscContactIgnorable);
      cPyIf::register_func("GetPscContactName",      pyGetPscContactName);
      cPyIf::register_func("GetPscContactGroup",     pyGetPscContactGroup);
      cPyIf::register_func("GetPscContactSubcGroup", pyGetPscContactSubcGroup);
      cPyIf::register_func("GetPscContactSubc",      pyGetPscContactSubc);
      cPyIf::register_func("GetPscContactSubcName",  pyGetPscContactSubcName);
      cPyIf::register_func("GetPscContactSubcIndex", pyGetPscContactSubcIndex);
      cPyIf::register_func("GetPscContactSubcIdNum", pyGetPscContactSubcIdNum);
      cPyIf::register_func("GetPscContactSubcInstName",
                                                   pyGetPscContactSubcInstName);

      // Electrical Devices
      cPyIf::register_func("ListElecDevs",           pyListElecDevs);
      cPyIf::register_func("SetEdevProperty",        pySetEdevProperty);
      cPyIf::register_func("GetEdevProperty",        pyGetEdevProperty);
      cPyIf::register_func("GetEdevObj",             pyGetEdevObj);

      // Resistance Extraction
      cPyIf::register_func("ExtractRL",              pyExtractRL );
      cPyIf::register_func("ExtractNetResistance",   pyExtractNetResistance);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // Python wrappers.

    // Menu Commands
    TCL_FUNC(DumpPhysNetlist,        4,  IFdumpPhysNetlist);
    TCL_FUNC(DumpElecNetlist,        4,  IFdumpElecNetlist);
    TCL_FUNC(SourceSpice,            2,  IFsourceSpice);
    TCL_FUNC(ExtractAndSet,          2,  IFextractAndSet);
    TCL_FUNC(FindPath,               4,  IFfindPath);
    TCL_FUNC(FindPathOfGroup,        2,  IFfindPathOfGroup);

    // Terminals
    TCL_FUNC(ListTerminals,          0,  IFlistTerminals);
    TCL_FUNC(FindTerminal,           8,  IFfindTerminal);
    TCL_FUNC(CreateTerminal,         4,  IFcreateTerminal);
    TCL_FUNC(DestroyTerminal,        1,  IFdestroyTerminal);
    TCL_FUNC(GetTerminalName,        1,  IFgetTerminalName);
    TCL_FUNC(SetTerminalName,        2,  IFsetTerminalName);
    TCL_FUNC(GetTerminalType,        1,  IFgetTerminalType);
    TCL_FUNC(SetTerminalType,        2,  IFsetTerminalType);
    TCL_FUNC(GetTerminalFlags,       1,  IFgetTerminalFlags);
    TCL_FUNC(SetTerminalFlags,       2,  IFsetTerminalFlags);
    TCL_FUNC(UnsetTerminalFlags,     2,  IFunsetTerminalFlags);
    TCL_FUNC(GetElecTerminalLoc,     3,  IFgetElecTerminalLoc);
    TCL_FUNC(SetElecTerminalLoc,     3,  IFsetElecTerminalLoc);
    TCL_FUNC(ClearElecTerminalLoc,   3,  IFclearElecTerminalLoc);

    // Physical Terminals
    TCL_FUNC(ListPhysTerminals,      0,  IFlistPhysTerminals);
    TCL_FUNC(FindPhysTerminal,       4,  IFfindPhysTerminal);
    TCL_FUNC(CreatePhysTerminal,     4,  IFcreatePhysTerminal);
    TCL_FUNC(HasPhysTerminal,        1,  IFhasPhysTerminal);
    TCL_FUNC(DestroyPhysTerminal,    1,  IFdestroyPhysTerminal);
    TCL_FUNC(GetPhysTerminalLoc,     2,  IFgetPhysTerminalLoc);
    TCL_FUNC(SetPhysTerminalLoc,     3,  IFsetPhysTerminalLoc);
    TCL_FUNC(GetPhysTerminalLayer,   1,  IFgetPhysTerminalLayer);
    TCL_FUNC(SetPhysTerminalLayer,   2,  IFsetPhysTerminalLayer);
    TCL_FUNC(GetPhysTerminalGroup,   1,  IFgetPhysTerminalGroup);
    TCL_FUNC(GetPhysTerminalObject,  1,  IFgetPhysTerminalObject);

    // Physical Conductor Groups
    TCL_FUNC(Group,                  0,  IFgroup);
    TCL_FUNC(GetNumberGroups,        0,  IFgetNumberGroups);
    TCL_FUNC(GetGroupBB,             2,  IFgetGroupBB);
    TCL_FUNC(GetGroupNode,           1,  IFgetGroupNode);
    TCL_FUNC(GetGroupName,           1,  IFgetGroupName);
    TCL_FUNC(GetGroupNetName,        1,  IFgetGroupNetName);
    TCL_FUNC(GetGroupCapacitance,    1,  IFgetGroupCapacitance);
    TCL_FUNC(CountGroupObjects,      1,  IFcountGroupObjects);
    TCL_FUNC(ListGroupObjects,       1,  IFlistGroupObjects);
    TCL_FUNC(CountGroupVias,         1,  IFcountGroupVias);
    TCL_FUNC(ListGroupVias,          1,  IFlistGroupVias);
    TCL_FUNC(CountGroupDevContacts,  1,  IFcountGroupDevContacts);
    TCL_FUNC(ListGroupDevContacts,   1,  IFlistGroupDevContacts);
    TCL_FUNC(CountGroupSubcContacts, 1,  IFcountGroupSubcContacts);
    TCL_FUNC(ListGroupSubcContacts,  1,  IFlistGroupSubcContacts);
    TCL_FUNC(CountGroupTerminals,    1,  IFcountGroupTerminals);
    TCL_FUNC(ListGroupTerminals,     1,  IFlistGroupTerminals);
    TCL_FUNC(CountGroupPhysTerminals,1,  IFcountGroupPhysTerminals);
    TCL_FUNC(ListGroupPhysTerminals, 1,  IFlistGroupPhysTerminals);
    TCL_FUNC(ListGroupTerminalNames, 1,  IFlistGroupTerminalNames);

    // Physical Devices
    TCL_FUNC(ListPhysDevs,           4,  IFlistPhysDevs);
    TCL_FUNC(GetPdevName,            1,  IFgetPdevName);
    TCL_FUNC(GetPdevIndex,           1,  IFgetPdevIndex);
    TCL_FUNC(GetPdevDual,            1,  IFgetPdevDual);
    TCL_FUNC(GetPdevBB,              2,  IFgetPdevBB);
    TCL_FUNC(GetPdevMeasure,         2,  IFgetPdevMeasure);
    TCL_FUNC(ListPdevMeasures,       1,  IFlistPdevMeasures);
    TCL_FUNC(ListPdevContacts,       1,  IFlistPdevContacts);
    TCL_FUNC(GetPdevContactName,     1,  IFgetPdevContactName);
    TCL_FUNC(GetPdevContactBB,       2,  IFgetPdevContactBB);
    TCL_FUNC(GetPdevContactGroup,    1,  IFgetPdevContactGroup);
    TCL_FUNC(GetPdevContactLayer,    1,  IFgetPdevContactLayer);
    TCL_FUNC(GetPdevContactDev,      1,  IFgetPdevContactDev);
    TCL_FUNC(GetPdevContactDevName,  1,  IFgetPdevContactDevName);
    TCL_FUNC(GetPdevContactDevIndex, 1,  IFgetPdevContactDevIndex);

    // Physical Subcircuits
    TCL_FUNC(ListPhysSubckts,        6,  IFlistPhysSubckts);
    TCL_FUNC(GetPscName,             1,  IFgetPscName);
    TCL_FUNC(GetPscIndex,            1,  IFgetPscIndex);
    TCL_FUNC(GetPscIdNum,            1,  IFgetPscIdNum);
    TCL_FUNC(GetPscInstName,         1,  IFgetPscInstName);
    TCL_FUNC(GetPscDual,             1,  IFgetPscDual);
    TCL_FUNC(GetPscBB,               2,  IFgetPscBB);
    TCL_FUNC(GetPscLoc,              2,  IFgetPscLoc);
    TCL_FUNC(GetPscTransform,        3,  IFgetPscTransform);
    TCL_FUNC(ListPscContacts,        1,  IFlistPscContacts);
    TCL_FUNC(IsPscContactIgnorable,  1,  IFisPscContactIgnorable);
    TCL_FUNC(GetPscContactName,      1,  IFgetPscContactName);
    TCL_FUNC(GetPscContactGroup,     1,  IFgetPscContactGroup);
    TCL_FUNC(GetPscContactSubcGroup, 1,  IFgetPscContactSubcGroup);
    TCL_FUNC(GetPscContactSubc,      1,  IFgetPscContactSubc);
    TCL_FUNC(GetPscContactSubcName,  1,  IFgetPscContactSubcName);
    TCL_FUNC(GetPscContactSubcIndex, 1,  IFgetPscContactSubcIndex);
    TCL_FUNC(GetPscContactSubcIdNum, 1,  IFgetPscContactSubcIdNum);
    TCL_FUNC(GetPscContactSubcInstName, 1, IFgetPscContactSubcInstName);

    // Electrical Devices
    TCL_FUNC(ListElecDevs,           1,  IFlistElecDevs);
    TCL_FUNC(SetEdevProperty,        3,  IFsetEdevProperty);
    TCL_FUNC(GetEdevProperty,        2,  IFgetEdevProperty);
    TCL_FUNC(GetEdevObj,             1,  IFgetEdevObj);

    // Resistance Extraction
    TCL_FUNC(ExtractRL,        VARARGS,  IFextractRL );
    TCL_FUNC(ExtractNetResistance, VARARGS, IFextractNetResistance);

    void tcl_register_extract()
    {
      // Menu Commands
      cTclIf::register_func("DumpPhysNetlist",        tclDumpPhysNetlist);
      cTclIf::register_func("DumpElecNetlist",        tclDumpElecNetlist);
      cTclIf::register_func("SourceSpice",            tclSourceSpice);
      cTclIf::register_func("ExtractAndSet",          tclExtractAndSet);
      cTclIf::register_func("FindPath",               tclFindPath);
      cTclIf::register_func("FindPathOfGroup",        tclFindPathOfGroup);

      // Terminals
      cTclIf::register_func("ListTerminals",          tclListTerminals);
      cTclIf::register_func("FindTerminal",           tclFindTerminal);
      cTclIf::register_func("CreateTerminal",         tclCreateTerminal);
      cTclIf::register_func("DestroyTerminal",        tclDestroyTerminal);
      cTclIf::register_func("GetTerminalName",        tclGetTerminalName);
      cTclIf::register_func("SetTerminalName",        tclSetTerminalName);
      cTclIf::register_func("GetTerminalType",        tclGetTerminalType);
      cTclIf::register_func("SetTerminalType",        tclSetTerminalType);
      cTclIf::register_func("GetTerminalFlags",       tclGetTerminalFlags);
      cTclIf::register_func("SetTerminalFlags",       tclSetTerminalFlags);
      cTclIf::register_func("UnsetTerminalFlags",     tclUnsetTerminalFlags);
      cTclIf::register_func("GetElecTerminalLoc",     tclGetElecTerminalLoc);
      cTclIf::register_func("SetElecTerminalLoc",     tclSetElecTerminalLoc);
      cTclIf::register_func("ClearElecTerminalLoc",   tclClearElecTerminalLoc);

      // Physical Terminals
      cTclIf::register_func("ListPhysTerminals",      tclListPhysTerminals);
      cTclIf::register_func("FindPhysTerminal",       tclFindPhysTerminal);
      cTclIf::register_func("CreatePhysTerminal",     tclCreatePhysTerminal);
      cTclIf::register_func("HasPhysTerminal",        tclHasPhysTerminal);
      cTclIf::register_func("DestroyPhysTerminal",    tclDestroyPhysTerminal);
      cTclIf::register_func("GetPhysTerminalLoc",     tclGetPhysTerminalLoc);
      cTclIf::register_func("SetPhysTerminalLoc",     tclSetPhysTerminalLoc);
      cTclIf::register_func("GetPhysTerminalLayer",   tclGetPhysTerminalLayer);
      cTclIf::register_func("SetPhysTerminalLayer",   tclSetPhysTerminalLayer);
      cTclIf::register_func("GetPhysTerminalGroup",   tclGetPhysTerminalGroup);
      cTclIf::register_func("GetPhysTerminalObject",  tclGetPhysTerminalObject);

      // Physical Conductor Groups
      cTclIf::register_func("Group",                  tclGroup);
      cTclIf::register_func("GetNumberGroups",        tclGetNumberGroups);
      cTclIf::register_func("GetGroupBB",             tclGetGroupBB);
      cTclIf::register_func("GetGroupNode",           tclGetGroupNode);
      cTclIf::register_func("GetGroupName",           tclGetGroupName);
      cTclIf::register_func("GetGroupNetName",        tclGetGroupNetName);
      cTclIf::register_func("GetGroupCapacitance",    tclGetGroupCapacitance);
      cTclIf::register_func("CountGroupObjects",      tclCountGroupObjects);
      cTclIf::register_func("ListGroupObjects",       tclListGroupObjects);
      cTclIf::register_func("CountGroupVias",         tclCountGroupVias);
      cTclIf::register_func("ListGroupVias",          tclListGroupVias);
      cTclIf::register_func("CountGroupDevContacts",  tclCountGroupDevContacts);
      cTclIf::register_func("ListGroupDevContacts",   tclListGroupDevContacts);
      cTclIf::register_func("CountGroupSubcContacts", tclCountGroupSubcContacts);
      cTclIf::register_func("ListGroupSubcContacts",  tclListGroupSubcContacts);
      cTclIf::register_func("CountGroupTerminals",    tclCountGroupTerminals);
      cTclIf::register_func("ListGroupTerminals",     tclListGroupTerminals);
      cTclIf::register_func("CountGroupPhysTerminals",tclCountGroupPhysTerminals);
      cTclIf::register_func("ListGroupPhysTerminals", tclListGroupPhysTerminals);
      cTclIf::register_func("ListGroupTerminalNames", tclListGroupTerminalNames);

      // Physical Devices
      cTclIf::register_func("ListPhysDevs",           tclListPhysDevs);
      cTclIf::register_func("GetPdevName",            tclGetPdevName);
      cTclIf::register_func("GetPdevIndex",           tclGetPdevIndex);
      cTclIf::register_func("GetPdevDual",            tclGetPdevDual);
      cTclIf::register_func("GetPdevBB",              tclGetPdevBB);
      cTclIf::register_func("GetPdevMeasure",         tclGetPdevMeasure);
      cTclIf::register_func("ListPdevMeasures",       tclListPdevMeasures);
      cTclIf::register_func("ListPdevContacts",       tclListPdevContacts);
      cTclIf::register_func("GetPdevContactName",     tclGetPdevContactName);
      cTclIf::register_func("GetPdevContactBB",       tclGetPdevContactBB);
      cTclIf::register_func("GetPdevContactGroup",    tclGetPdevContactGroup);
      cTclIf::register_func("GetPdevContactLayer",    tclGetPdevContactLayer);
      cTclIf::register_func("GetPdevContactDev",      tclGetPdevContactDev);
      cTclIf::register_func("GetPdevContactDevName",  tclGetPdevContactDevName);
      cTclIf::register_func("GetPdevContactDevIndex", tclGetPdevContactDevIndex);

      // Physical Subcircuits
      cTclIf::register_func("ListPhysSubckts",        tclListPhysSubckts);
      cTclIf::register_func("GetPscName",             tclGetPscName);
      cTclIf::register_func("GetPscIndex",            tclGetPscIndex);
      cTclIf::register_func("GetPscIdNum",            tclGetPscIdNum);
      cTclIf::register_func("GetPscInstName",         tclGetPscInstName);
      cTclIf::register_func("GetPscDual",             tclGetPscDual);
      cTclIf::register_func("GetPscBB",               tclGetPscBB);
      cTclIf::register_func("GetPscLoc",              tclGetPscLoc);
      cTclIf::register_func("GetPscTransform",        tclGetPscTransform);
      cTclIf::register_func("ListPscContacts",        tclListPscContacts);
      cTclIf::register_func("IsPscContactIgnorable",  tclIsPscContactIgnorable);
      cTclIf::register_func("GetPscContactName",      tclGetPscContactName);
      cTclIf::register_func("GetPscContactGroup",     tclGetPscContactGroup);
      cTclIf::register_func("GetPscContactSubcGroup", tclGetPscContactSubcGroup);
      cTclIf::register_func("GetPscContactSubc",      tclGetPscContactSubc);
      cTclIf::register_func("GetPscContactSubcName",  tclGetPscContactSubcName);
      cTclIf::register_func("GetPscContactSubcIndex", tclGetPscContactSubcIndex);
      cTclIf::register_func("GetPscContactSubcIdNum", tclGetPscContactSubcIdNum);
      cTclIf::register_func("GetPscContactSubcInstName",
                                                   tclGetPscContactSubcInstName);

      // Electrical Devices
      cTclIf::register_func("ListElecDevs",           tclListElecDevs);
      cTclIf::register_func("SetEdevProperty",        tclSetEdevProperty);
      cTclIf::register_func("GetEdevProperty",        tclGetEdevProperty);
      cTclIf::register_func("GetEdevObj",             tclGetEdevObj);

      // Resistance Extraction
      cTclIf::register_func("ExtractRL",              tclExtractRL );
      cTclIf::register_func("ExtractNetResistance",   tclExtractNetResistance);
    }
#endif  // HAVE_TCL
}


// Export to load functions in this script library.
//
void
cExt::loadScriptFuncs()
{
  using namespace extract_funcs;

  // Menu Commands
  SIparse()->registerFunc("DumpPhysNetlist",        4,  IFdumpPhysNetlist);
  SIparse()->registerFunc("DumpElecNetlist",        4,  IFdumpElecNetlist);
  SIparse()->registerFunc("SourceSpice",            2,  IFsourceSpice);
  SIparse()->registerFunc("ExtractAndSet",          2,  IFextractAndSet);
  SIparse()->registerFunc("FindPath",               4,  IFfindPath);
  SIparse()->registerFunc("FindPathOfGroup",        2,  IFfindPathOfGroup);

  // Terminals
  SIparse()->registerFunc("ListTerminals",          0,  IFlistTerminals);
  SIparse()->registerFunc("FindTerminal",           8,  IFfindTerminal);
  SIparse()->registerFunc("CreateTerminal",         4,  IFcreateTerminal);
  SIparse()->registerFunc("DestroyTerminal",        1,  IFdestroyTerminal);
  SIparse()->registerFunc("GetTerminalName",        1,  IFgetTerminalName);
  SIparse()->registerFunc("SetTerminalName",        2,  IFsetTerminalName);
  SIparse()->registerFunc("GetTerminalType",        1,  IFgetTerminalType);
  SIparse()->registerFunc("SetTerminalType",        2,  IFsetTerminalType);
  SIparse()->registerFunc("GetTerminalFlags",       1,  IFgetTerminalFlags);
  SIparse()->registerFunc("SetTerminalFlags",       2,  IFsetTerminalFlags);
  SIparse()->registerFunc("UnsetTerminalFlags",     2,  IFunsetTerminalFlags);
  SIparse()->registerFunc("GetElecTerminalLoc",     3,  IFgetElecTerminalLoc);
  SIparse()->registerFunc("SetElecTerminalLoc",     3,  IFsetElecTerminalLoc);
  SIparse()->registerFunc("ClearElecTerminalLoc",   3,  IFclearElecTerminalLoc);

  // Physical Terminals
  SIparse()->registerFunc("ListPhysTerminals",      0,  IFlistPhysTerminals);
  SIparse()->registerFunc("FindPhysTerminal",       4,  IFfindPhysTerminal);
  SIparse()->registerFunc("CreatePhysTerminal",     4,  IFcreatePhysTerminal);
  SIparse()->registerFunc("HasPhysTerminal",        1,  IFhasPhysTerminal);
  SIparse()->registerFunc("DestroyPhysTerminal",    1,  IFdestroyPhysTerminal);
  SIparse()->registerFunc("GetPhysTerminalLoc",     2,  IFgetPhysTerminalLoc);
  SIparse()->registerFunc("SetPhysTerminalLoc",     3,  IFsetPhysTerminalLoc);
  SIparse()->registerFunc("GetPhysTerminalLayer",   1,  IFgetPhysTerminalLayer);
  SIparse()->registerFunc("SetPhysTerminalLayer",   2,  IFsetPhysTerminalLayer);
  SIparse()->registerFunc("GetPhysTerminalGroup",   1,  IFgetPhysTerminalGroup);
  SIparse()->registerFunc("GetPhysTerminalObject",  1,  IFgetPhysTerminalObject);

  // Physical Conductor Groups
  SIparse()->registerFunc("Group",                  0,  IFgroup);
  SIparse()->registerFunc("GetNumberGroups",        0,  IFgetNumberGroups);
  SIparse()->registerFunc("GetGroupBB",             2,  IFgetGroupBB);
  SIparse()->registerFunc("GetGroupNode",           1,  IFgetGroupNode);
  SIparse()->registerFunc("GetGroupName",           1,  IFgetGroupName);
  SIparse()->registerFunc("GetGroupNetName",        1,  IFgetGroupNetName);
  SIparse()->registerFunc("GetGroupCapacitance",    1,  IFgetGroupCapacitance);
  SIparse()->registerFunc("CountGroupObjects",      1,  IFcountGroupObjects);
  SIparse()->registerFunc("ListGroupObjects",       1,  IFlistGroupObjects);
  SIparse()->registerFunc("CountGroupVias",         1,  IFcountGroupVias);
  SIparse()->registerFunc("ListGroupVias",          1,  IFlistGroupVias);
  SIparse()->registerFunc("CountGroupDevContacts",  1,  IFcountGroupDevContacts);
  SIparse()->registerFunc("ListGroupDevContacts",   1,  IFlistGroupDevContacts);
  SIparse()->registerFunc("CountGroupSubcContacts", 1,  IFcountGroupSubcContacts);
  SIparse()->registerFunc("ListGroupSubcContacts",  1,  IFlistGroupSubcContacts);
  SIparse()->registerFunc("CountGroupTerminals",    1,  IFcountGroupTerminals);
  SIparse()->registerFunc("ListGroupTerminals",     1,  IFlistGroupTerminals);
  SIparse()->registerFunc("CountGroupPhysTerminals",1,  IFcountGroupPhysTerminals);
  SIparse()->registerFunc("ListGroupPhysTerminals", 1,  IFlistGroupPhysTerminals);
  SIparse()->registerFunc("ListGroupTerminalNames", 1,  IFlistGroupTerminalNames);

  // Physical Devices
  SIparse()->registerFunc("ListPhysDevs",           4,  IFlistPhysDevs);
  SIparse()->registerFunc("GetPdevName",            1,  IFgetPdevName);
  SIparse()->registerFunc("GetPdevIndex",           1,  IFgetPdevIndex);
  SIparse()->registerFunc("GetPdevDual",            1,  IFgetPdevDual);
  SIparse()->registerFunc("GetPdevBB",              2,  IFgetPdevBB);
  SIparse()->registerFunc("GetPdevMeasure",         2,  IFgetPdevMeasure);
  SIparse()->registerFunc("ListPdevMeasures",       1,  IFlistPdevMeasures);
  SIparse()->registerFunc("ListPdevContacts",       1,  IFlistPdevContacts);
  SIparse()->registerFunc("GetPdevContactName",     1,  IFgetPdevContactName);
  SIparse()->registerFunc("GetPdevContactBB",       2,  IFgetPdevContactBB);
  SIparse()->registerFunc("GetPdevContactGroup",    1,  IFgetPdevContactGroup);
  SIparse()->registerFunc("GetPdevContactLayer",    1,  IFgetPdevContactLayer);
  SIparse()->registerFunc("GetPdevContactDev",      1,  IFgetPdevContactDev);
  SIparse()->registerFunc("GetPdevContactDevName",  1,  IFgetPdevContactDevName);
  SIparse()->registerFunc("GetPdevContactDevIndex", 1,  IFgetPdevContactDevIndex);

  // Physical Subcircuits
  SIparse()->registerFunc("ListPhysSubckts",        6,  IFlistPhysSubckts);
  SIparse()->registerFunc("GetPscName",             1,  IFgetPscName);
  SIparse()->registerFunc("GetPscIndex",            1,  IFgetPscIndex);
  SIparse()->registerFunc("GetPscIdNum",            1,  IFgetPscIdNum);
  SIparse()->registerFunc("GetPscInstName",         1,  IFgetPscInstName);
  SIparse()->registerFunc("GetPscDual",             1,  IFgetPscDual);
  SIparse()->registerFunc("GetPscBB",               2,  IFgetPscBB);
  SIparse()->registerFunc("GetPscLoc",              2,  IFgetPscLoc);
  SIparse()->registerFunc("GetPscTransform",        3,  IFgetPscTransform);
  SIparse()->registerFunc("ListPscContacts",        1,  IFlistPscContacts);
  SIparse()->registerFunc("IsPscContactIgnorable",  1,  IFisPscContactIgnorable);
  SIparse()->registerFunc("GetPscContactName",      1,  IFgetPscContactName);
  SIparse()->registerFunc("GetPscContactGroup",     1,  IFgetPscContactGroup);
  SIparse()->registerFunc("GetPscContactSubcGroup", 1, IFgetPscContactSubcGroup);
  SIparse()->registerFunc("GetPscContactSubc",      1,  IFgetPscContactSubc);
  SIparse()->registerFunc("GetPscContactSubcName",  1,  IFgetPscContactSubcName);
  SIparse()->registerFunc("GetPscContactSubcIndex", 1, IFgetPscContactSubcIndex);
  SIparse()->registerFunc("GetPscContactSubcIdNum", 1, IFgetPscContactSubcIdNum);
  SIparse()->registerFunc("GetPscContactSubcInstName", 1,
                                                    IFgetPscContactSubcInstName);

  // Electrical Devices
  SIparse()->registerFunc("ListElecDevs",           1,  IFlistElecDevs);
  SIparse()->registerFunc("SetEdevProperty",        3,  IFsetEdevProperty);
  SIparse()->registerFunc("GetEdevProperty",        2,  IFgetEdevProperty);
  SIparse()->registerFunc("GetEdevObj",             1,  IFgetEdevObj);

  // Resistance Extraction
  SIparse()->registerFunc("ExtractRL",        VARARGS,  IFextractRL );
  SIparse()->registerFunc("ExtractNetResistance", VARARGS,
                                                        IFextractNetResistance);

#ifdef HAVE_PYTHON
  py_register_extract();
#endif
#ifdef HAVE_TCL
  tcl_register_extract();
#endif
}

#define FREE_CHK_ZL(b, zl) { if (b) Zlist::destroy(zl); zl = 0; }


/*========================================================================*
 * Extraction Functions
 *========================================================================*/
//-------------------------------------------------------------------------
// Menu Commands
//-------------------------------------------------------------------------

// (int) DumpPhysNetlist(filename, depth, modestring, names)
//
// This function dumps a netlist file extracted from the physical part
// of the database, much like the Dump Phys Netlist command in the
// Extract Menu.  The filename argument is a file name which will
// receive the output.  If null or empty, the file will be the base
// name of the current cell with ".physnet" appended.  The depth
// argument specifies the depth of the hierarchy to process.  If an
// integer, 0 represents the current cell only, 1 includes the first
// level subcells, etc.  A negative integer specifies to process the
// entire hierarchy.  This argument can also be a string beginning
// with the letter 'a', which will process all levels of the
// hierarchy.
//
// The third argument is a string, consisting of characters from the
// table below, which set the mode of the command.  These are
// analogous to the check boxes that appear with the Dump Phys Netlist
// command.  If a character does not appear in the string, that option
// is turned off.  If it appears in lower case, the option is turned
// on, and if it appears in upper case, the option will be set by the
// present value of the corresponding "!set" variable.  The characters
// can appear in any order.
//
// character  option            corresponding variable
// n          net               NoPnet
// d          devs              NoPnetDevs
// s          spice             NoPnetSpice
// b          list bottom-up    PnetBottomUp
// g          show geometry     PnetShowGeometry
// c          include wire cap  PnetIncludeWireCap
// a          list all cells    PnetListAll
// l          ignore labels     PnetNoLabels
//
// The final argument, if not null or empty, contains a
// space-separated list of physical format names, each of which must
// match a PnetFormat name in the format library file, or option names
// from the table above.  The names that contain white space should be
// double-quoted.
//
// For each cell, a field in the output is generated for each format
// choice implicit in the modestring or given in the names.  In most
// cases, only one format is probably wanted.  The option text in the
// table above can also be included in the names, which is equivalent
// to giving the corresponding lower-case letter in the modestring.
// The modestring setting will have precedence if there is a conflict.
// If both the modestring and the names string are empty or null, an
// effecitve mode string consisting of all of the upper-case option
// letters is used.
//
//  Example:  print a SPICE file
//    DumpPhysNetlist("myfile.cir", "a", "s", 0)
//    or
//    DumpPhysNetlist("myfile.cir", "a", 0, "spice")
//
// If the function succeeds, 1 is returned, otherwise 0 is returned.
//
bool
extract_funcs::IFdumpPhysNetlist(Variable *res, Variable *args, void*)
{
    static char defmode[] = "NDSBGCA";
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))
    int depth;
    ARG_CHK(arg_depth(args, 1, &depth))
    const char *modestr;
    ARG_CHK(arg_string(args, 2, &modestr))
    const char *names;
    ARG_CHK(arg_string(args, 3, &names))
    if ((!modestr || !*modestr) && (!names && !*names))
        modestr = defmode;

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    bool fnset = true;
    if (!fname) {
        char buf[128];
        strcpy(buf, Tstring(DSP()->CurCellName()));
        char *t = strrchr(buf, '.');
        if (t && t != buf)
            *t = 0;
        strcat(buf, ".physnet");
        fname = lstring::copy(buf);
        fnset = false;
    }
    sDumpOpts *opts = EX()->pnetNetOpts(modestr, names);
    opts->set_depth(depth);

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (filestat::create_bak(fname)) {
        FILE *fp = fopen(fname, "w");
        if (fp) {
            res->content.value = EX()->dumpPhysNetlist(fp, cursdp, opts);
            fclose(fp);
        }
    }
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
    if (!fnset)
        delete [] fname;
    delete opts;
    return (OK);
}


// (int) DumpElecNetlist(filename, depth, modestring, names)
//
// This function dumps a netlist file extracted from the electrical
// part of the database, much like the Dump Elec Netlist command in
// the Extract Menu.  The filename argument is a file name which will
// receive the output.  If null or empty, the file will be the base
// name of the current cell with ".elecnet" appended.  The depth
// argument specifies the depth of the hierarchy to process.  If an
// integer, 0 represents the current cell only, 1 includes the first
// level subcells, etc.  A negative integer specifies to process the
// entire hierarchy.  This argument can also be a string beginning
// with the letter 'a', which will process all levels of the
// hierarchy.
//
// The third argument is a string, consisting of characters from the
// table below, which set the mode of the command.  These are
// analogous to the check boxes that appear with the Dump Elec Netlist
// command.  If a character does not appear in the string, that option
// is turned off.  If it appears in lower case, the option is turned
// on, and if it appears in upper case, the option will be set by the
// present value of the corresponding "!set" variable.  The characters
// can appear in any order.  If the string is empty or null, all
// options will be set by the corresponding variables.
//
// character  option            corresponding variable
// n          net               NoEnet
// s          spice             EnetSpice
// b          list bottom-up    EnetBottomUp
//
// The final argument, if not null or empty, contains a
// space-separated list of electrical format names, each of which must
// match an EnetFormat name in the format library file, or option
// names from the table above.  The names that contain white space
// should be double-quoted.
//
// For each cell, a field in the output is generated for each format
// choice implicit in the modestring or given in the names.  In most
// cases, only one format is probably wanted.  The option text in the
// table above can also be included in the names, which is equivalent
// to giving the corresponding lower-case letter in the modestring.
// The modestring setting will have precedence if there is a conflict.
// If both the modestring and the names string are empty or null, an
// effecitve mode string consisting of all of the upper-case option
// letters is used.
//
// If the function succeeds, 1 is returned, otherwise 0 is returned.
//
bool
extract_funcs::IFdumpElecNetlist(Variable *res, Variable *args, void*)
{
    static char defmode[] = "NSB";
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))
    int depth;
    ARG_CHK(arg_depth(args, 1, &depth))
    const char *modestr;
    ARG_CHK(arg_string(args, 2, &modestr))
    const char *names;
    ARG_CHK(arg_string(args, 3, &names))
    if ((!modestr || !*modestr) && (!names && !*names))
        modestr = defmode;

    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return (BAD);
    bool fnset = true;
    if (!fname) {
        char buf[128];
        strcpy(buf, Tstring(DSP()->CurCellName()));
        char *t = strrchr(buf, '.');
        if (t && t != buf)
            *t = 0;
        strcat(buf, ".elecnet");
        fname = lstring::copy(buf);
        fnset = false;
    }

    sDumpOpts *opts = EX()->enetNewOpts(modestr, names);
    opts->set_depth(depth);

    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (filestat::create_bak(fname)) {
        FILE *fp = fopen(fname, "w");
        if (fp) {
            res->content.value = EX()->dumpElecNetlist(fp, cursde, opts);
            fclose(fp);
        }
    }
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
    if (!fnset)
        delete [] fname;
    delete opts;
    return (OK);
}


// (int) SourceSpice(filename, modestring)
//
// This function will parse a SPICE file, adding to or updating the
// electrical part of the database with the devices and subcircuits
// found.  This is equivalent to the Source SPICE command in the
// Extract Menu.  The first argument is a path to the SPICE file to
// process.
//
// The final argument is a string, consisting of characters from the
// table below, which set the mode of the command.  These are
// analogous to the check boxes that appear with the Source SPICE
// command.  If a character does not appear in the string, that option
// is turned off.  If it appears in lower case, the option is turned
// on, and if it appears in upper case, the option will be set by the
// present value of the corresponding "!set" variable.  The characters
// can appear in any order.  If the string is empty or null, all
// options will be set by the corresponding variables.
//
// character  option            corresponding variable
// a          all devs          SourceAllDevs
// r          create            SourceCreate
// l          clear             SourceClear
//
// If the operation succeeds, 1 is returned, otherwise 0 is returned.
//
bool
extract_funcs::IFsourceSpice(Variable *res, Variable *args, void*)
{
    static char defmode[] = "ARL";
    const char *fname;
    ARG_CHK(arg_string(args, 0, &fname))
    const char *modestr;
    ARG_CHK(arg_string(args, 1, &modestr))
    if (!modestr || !*modestr)
        modestr = defmode;

    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return (BAD);
    if (!fname || !*fname)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    FILE *fp = fopen(fname, "fp");
    if (fp) {
        int mode = 0;
        if (strchr(modestr, 'a'))
            mode |= EFS_ALLDEVS;
        else if (strchr(modestr, 'A') &&
                CDvdb()->getVariable(VA_SourceAllDevs))
            mode |= EFS_ALLDEVS;
        if (strchr(modestr, 'r'))
            mode |= EFS_CREATE;
        else if (strchr(modestr, 'R') &&
                CDvdb()->getVariable(VA_SourceCreate))
            mode |= EFS_CREATE;
        if (strchr(modestr, 'l'))
            mode |= EFS_CLEAR;
        else if (strchr(modestr, 'L') &&
                CDvdb()->getVariable(VA_SourceClear))
            mode |= EFS_CLEAR;
        SCD()->extractFromSpice(cursde, fp, mode);
        fclose(fp);
        res->content.value = 1;
    }
    return (OK);
}


// (int) ExtractAndSet(depth, modestring)
//
// This function performs extraction on the physical part of the
// database, updating the electrical part.  This is equivalent to the
// Source Physical command in the Extract Menu.  The first argument
// indicates the depth of the hierarchy to process.  This can be an
// integer:  0 means process the current cell only, 1 means process
// the current cell plus the subcells, etc., and a negative integer
// sets the depth to process the entire hierarchy.  This argument can
// also be a string starting with 'a' such as "a" or "all" which
// indicates to process the entire hierarchy.
//
// The final argument is a string, consisting of characters from the
// table below, which set the mode of the command.  These are
// analogous to the check boxes that appear with the Source Physical
// command.  If a character does not appear in the string, that option
// is turned off.  If it appears in lower case, the option is turned
// on, and if it appears in upper case, the option will be set by the
// present value of the corresponding "!set" variable.  The characters
// can appear in any order.  If the string is empty or null, all
// options will be set by the corresponding variables.
//
// character  option            corresponding variable
// a          all devs          NoExsetAllDevs
// r          create            NoExsetCreate
// l          clear             ExsetClear
// c          include wire cap  ExsetIncludeWireCap
// n          ignore labels     ExsetNoLabels
//
// If the operation succeeds, 1 is returned, otherwise 0 is returned.
// This function does not redraw the windows.
//
bool
extract_funcs::IFextractAndSet(Variable *res, Variable *args, void*)
{
    static char defmode[] = "ARLCN";
    int depth;
    ARG_CHK(arg_depth(args, 0, &depth))
    const char *modestr;
    ARG_CHK(arg_string(args, 1, &modestr))
    if (!modestr || !*modestr)
        modestr = defmode;

    CDs *cursdp = CurCell(Physical, true);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    int mode = 0;
    if (strchr(modestr, 'a'))
        mode |= EFS_ALLDEVS;
    else if (strchr(modestr, 'A') &&
            !CDvdb()->getVariable(VA_NoExsetAllDevs))
        mode |= EFS_ALLDEVS;
    if (strchr(modestr, 'r'))
        mode |= EFS_CREATE;
    else if (strchr(modestr, 'R') &&
            !CDvdb()->getVariable(VA_NoExsetCreate))
        mode |= EFS_CREATE;
    if (strchr(modestr, 'l'))
        mode |= EFS_CLEAR;
    else if (strchr(modestr, 'L') &&
            CDvdb()->getVariable(VA_ExsetClear))
        mode |= EFS_CLEAR;
    if (strchr(modestr, 'c'))
        mode |= EFS_WIRECAP;
    else if (strchr(modestr, 'C') &&
            CDvdb()->getVariable(VA_ExsetIncludeWireCap))
        mode |= EFS_WIRECAP;
    if (strchr(modestr, 'n'))
        mode |= MEL_NOLABELS;
    else if (strchr(modestr, 'N') &&
            CDvdb()->getVariable(VA_ExsetNoLabels))
        mode |= MEL_NOLABELS;

    res->content.value = EX()->makeElec(cursdp, depth, mode);
    return (OK);
}


// (object_handle) FindPath(x, y, depth, use_extract)
//
// This function returns a handle to a list of copies of physical
// conducting objects in a wire net.  The x,y point (microns, in the
// physical part of the current cell) should intersect a conducting
// object, and the list will consist of this object plus connected
// objects.  The depth argument is an integer or a string beginning
// with "a" (for "all") which gives the hierarchy search depth.  Only
// objects in cells to this depth will be considered for addition to
// the list (0 means objects in the current cell only).  If the
// boolean value use_extract is nonzero, the main extraction functions
// will be used to determine the connectivity.  If the value is zero,
// the connectivity is established through geometry.  This is similar
// to the Select Path and "Quick" Path modes available in the extract
// Selections control panel.
//
// The return value is a handle to a list of object copies, or 0 if no
// objects are found.
//
bool
extract_funcs::IFfindPath(Variable *res, Variable *args, void*)
{
    int x;
    ARG_CHK(arg_coord(args, 0, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 1, &y, Physical))
    int depth;
    ARG_CHK(arg_depth(args, 2, &depth))
    bool use_ex;
    ARG_CHK(arg_boolean(args, 3, &use_ex))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    if (use_ex) {
        if (!EX()->extract(cursdp))
            return (BAD);
    }
    pathfinder *pf = use_ex ? new grp_pathfinder() : new pathfinder();
    pf->set_depth(depth);
    BBox BB(x, y, x, y);
    BB.bloat(5);
    if (!pf->find_path(&BB)) {
        res->type = TYP_SCALAR;
        res->content.value = 0;
        delete pf;
        return (OK);
    }

    CDol *ol0 = CDol::object_list(pf->get_object_list());
    if (ol0) {
        sHdl *nhdl = new sHdlObject(ol0, cursdp, true);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0;
    }
    delete pf;
    return (OK);
}


// (object_handle) FindPathOfGroup(groupnum, depth)
//
// This function returns a handle to a list of copies of physical
// conducting objects in the group number from the current cell given,
// to the given depth.  The depth argument is an integer or a string
// beginning with "a" (for "all") which gives the hierarchy search
// depth.  Only objects in cells to this depth will be considered for
// addition to the list (0 means objects in the current cell only).
//
// The function will fail (halt the script) on a major error.  If the
// group number is out of range, or a "minor" error occurs, the
// function will return a scalar 0, and an error message should be
// available from GetError.
//
// Otherwise, the return value is a handle to a list of object copies,
// or the list may be empty if the group has no physical objects.
//
bool
extract_funcs::IFfindPathOfGroup(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))
    int depth;
    ARG_CHK(arg_depth(args, 1, &depth))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        SIparse()->pushError("No current physical cell!");
        return (BAD);
    }
    if (!EX()->extract(cursdp)) {
        SIparse()->pushError("Extraction failed!");
        return (BAD);
    }

    grp_pathfinder pf;
    pf.set_depth(depth);
    cGroupDesc *gd = cursdp->groups();
    if (!gd) {
        SIparse()->pushError("No groups in current cell!");
        return (BAD);
    }

    res->type = TYP_SCALAR;
    res->content.value = 0;

    sGroup *gp = gd->group_for(group);
    if (!gp) {
        Errs()->add_error("bad group number.");
        return (OK);
    }
    sGroupObjs *go = gp->net();

    if (go && go->objlist() && !pf.find_path(go->objlist()->odesc)) {
        Errs()->add_error("path finder failed, unknown error.");
        return (OK);
    }

    CDol *ol0 = CDol::object_list(pf.get_object_list());
    if (ol0) {
        sHdl *nhdl = new sHdlObject(ol0, cursdp, true);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    else {
        sHdl *nhdl = new sHdlObject(0, cursdp, true);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Terminals  (these actually reference cell node properties)
//-------------------------------------------------------------------------

// (terminal_handle) ListTerminals()
//
// Return a handle containing a list of the connection terminals of
// the current cell.  These correspond to the normal contact terminals
// as would be defined with the subct command, as represented by
// <b>node</b> properties of the electrical cell view.  On success, a
// handle is returned containing the terminal list.  If there are no
// terminals defined or some other error occurs, a scalar 0 is
// returned.
//
bool
extract_funcs::IFlistTerminals(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (OK);
    CDp_snode *ps = (CDp_snode*)cursde->prpty(P_NODE);
    tlist2<CDp_nodeEx> *t0 = 0, *te = 0;
    for ( ; ps; ps = ps->next()) {
        if (!t0)
            t0 = te = new tlist2<CDp_nodeEx>(ps, cursde, 0);
        else {
            te->next = new tlist2<CDp_nodeEx>(ps, cursde, 0);
            te = te->next;
        }
    }
    if (t0) {
        sHdl *nhdl = new sHdlNode(t0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    return (OK);
}


// (terminal_handle) FindTerminal(name, index, use_e, xe, ye, use_p, xp, yp)
//
// This function will return a handle referencing a single terminal, if
// one can be found among the current cell contact terminals that
// matches the arguments.  The arguments specify parameters, any of
// which can be ignored.  The non-ignored parameters must all match.
//
// The name can be a string that will match an applied terminal name
// (not a default name generated by Xic).  The argument will be
// ignored if a scalar 0 or null or empty string is passed.
//
// The index is the terminal order index, or -1 if the parameter is to
// be ignored.  This is the number that is shown within the terminal
// box in the subct command.
//
// If use_e is a nonzero value, the next two arguments are taken as a
// location in the electrical drawing.  These are specified in
// fictitious "microns" which represent 1000 internal units.  These
// are the numbers displayed in the coordinate readout area while a
// schematic is being edited.  A location match will depend of whether
// the electrical cell is symbolic or not.  If symbolic, a location
// match to any of the placement locations will count as a match
// (terminals can have more than one "hot spot" in the symbolic
// display).  If use_e is 0, the two arguments that follow are ignored
// and can be any numeric values.
//
// Similarly, if use_p is nonzero, the next two arguments represent a
// coordinate in the layout, given in (real) microns.  If a physical
// terminal is placed at the given location, a match will be
// indicated.  If use_p is zero, the two arguments that follow are
// ignored, and can be set to any numeric values.
//
// The arguments should provide at least one matchable parameter. 
// Internally, the list of terminals is scanned, and the first
// matching terminal found is returned, referenced by a handle.  If no
// terminals match, a scalar zero is returned.
//
bool
extract_funcs::IFfindTerminal(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    int index;
    ARG_CHK(arg_int(args, 1, &index))
    bool use_e;
    ARG_CHK(arg_boolean(args, 2, &use_e))
    int xe;
    ARG_CHK(arg_coord(args, 3, &xe, Electrical))
    int ye;
    ARG_CHK(arg_coord(args, 4, &ye, Electrical))
    bool use_p;
    ARG_CHK(arg_boolean(args, 5, &use_p))
    int xp;
    ARG_CHK(arg_coord(args, 6, &xp, Physical))
    int yp;
    ARG_CHK(arg_coord(args, 7, &yp, Physical))

    Point_c pe(xe, ye);
    Point_c pp(xp, yp);
    CDp_snode *ps = EX()->findTerminal(name, index, use_e ? &pe : 0,
        use_p ? &pp : 0);
    if (ps) {
        tlist2<CDp_nodeEx> *t0 = new tlist2<CDp_nodeEx>(ps, CurCell(Electrical),
            0);
        sHdl *nhdl = new sHdlNode(t0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0;
    }
    return (OK);
}


// (terminal_handle) CreateTerminal(name, x, y, termtype)
//
// This function will create a new terminal in the schematic of the
// current cell.  If a name string is passed, the terminal will be
// given that name.  If this argument is a scalar 0 or a null or empty
// string, the terminal will not have an assigned name but will use an
// internally generated name.  The terminal will be placed at the
// location indicated by the x and y arguments, which are in
// fictitious "microns" representing 1000 database units.  These are
// the same coordinates as displayed in the coordinate readout while a
// schematic is being edited.
//
// The termtype argument can be a scalar integer or a keyword, from
// the list below.  This will assign a type to the terminal.  The type
// is not used by Xic, but this facility may be useful to the user.
//
//    0   input
//    1   output
//    2   inout
//    3   tristate
//    4   clock
//    5   outclock
//    6   supply
//    7   outsupply
//    8   ground
//
// Keyword matching is case-insensitive.  If the argument is not
// recognized, and the default "input" will be used.
//
// The function returns a handle that references the new terminal on
// success, or a scalar zero otherwise.
//
bool
extract_funcs::IFcreateTerminal(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, Electrical))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, Electrical))
    const char *termtype = 0;
    if (args[3].type == TYP_SCALAR) {
        int val;
        ARG_CHK(arg_int(args, 3, &val))
        for (FlagDef *f = TermTypes; f->name; f++) {
            if (f->value == (unsigned int)val) {
                termtype = f->name;
                break;
            }
        }
    }
    else {
        ARG_CHK(arg_string(args, 3, &termtype))
    }

    res->type = TYP_SCALAR;
    res->content.value = 0;
    Point_c pe(x, y);
    CDp_snode *ps = EX()->createTerminal(name, &pe, termtype);
    if (ps) {
        tlist2<CDp_nodeEx> *t0 = new tlist2<CDp_nodeEx>(ps, CurCell(Electrical),
            0);
        sHdl *nhdl = new sHdlNode(t0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0;
    }
    return (OK);
}


// (int) DestroyTerminal(thandle)
//
// This function will destroy the terminal referenced by the passed
// handle, and will close the handle.  This destroys the terminal,
// which is actually a node property of the electrical current cell,
// and the linkage into the physical layout, if any.  If a terminal
// was destroyed, value one is returned, or zero on error.
//
bool
extract_funcs::IFdestroyTerminal(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLnode)
            return (BAD);
        tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
        if (t && t->elt) {
            CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
            if (ps) {
                bool ok = EX()->destroyTerminal(ps);
                if (ok)
                    ok = !hdl->close(id);
                res->content.value = ok;
            }
        }
    }
    return (OK);
}


// (string) GetTerminalName(thandle)
//
// Return a string containing the name of the terminal or physical
// terminal referenced by the handle passed as an argument.  Both
// objects have name fields that track.  However, if no name was
// assigned, for a terminal a default name generated by Xic is
// returned, whereas the return from a physical terminal will be
// null.
//
bool
extract_funcs::IFgetTerminalName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                res->content.string =
                    lstring::copy(Tstring(t->elt->term_name()));
                res->flags |= VF_ORIGINAL;
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                res->content.string =
                    lstring::copy(Tstring(t->elt->name()));
                res->flags |= VF_ORIGINAL;
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (int) SetTerminalName(thandle, name)
//
// The first argument is a handle that references a terminal or a
// physical terminal.  The second argument is a string which gives a
// name to apply.  It can also be a scalar 0, or if null or empty any
// existing assigned name will be removed.  Both terminals and
// physical terminals have names that track, this will change both,
// when both objects exist.  The return value is one on success, zero
// if error.
//
bool
extract_funcs::IFsetTerminalName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *name;
    ARG_CHK(arg_string(args, 1, &name))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps) {
                    res->content.value = EX()->setTerminalName(
                        ps->cell_terminal(), name);
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term)
                    res->content.value = EX()->setTerminalName(term, name);
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (int) GetTerminalType(terminal_handle)
//
// Return a type code for the terminal referenced by the handle passed
// as an argument, which can also be a handle to the corresponding
// physical terminal.  A non-negative return represents success.  The
// code represents the terminal type set by the user.  The terminal
// type is not used by Xic, but is available for user applications. 
// The defined types are listed below.  The default is type 0.
//
//    0   input
//    1   output
//    2   inout
//    3   tristate
//    4   clock
//    5   outclock
//    6   supply
//    7   outsupply
//    9   ground
//
bool
extract_funcs::IFgetTerminalType(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt)
                res->content.value = t->elt->termtype();
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *sterm = dynamic_cast<CDsterm*>(t->elt);
                if (sterm) {
                    CDp_snode *ps = sterm->node_prpty();
                    if (ps)
                        res->content.value = ps->termtype();
                }
                else {
                    CDcterm *cterm = dynamic_cast<CDcterm*>(t->elt);
                    if (cterm) {
                        CDp_cnode *pc = cterm->node_prpty();
                        if (pc)
                            res->content.value = pc->termtype();
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (int) SetTerminalType(thandle, termtype)
//
// This function will apply a terminal type to the terminal referenced
// by the handle passed as the first argument, which can also be a
// handle to the corresponding physical terminal.  The second argument
// is either an integer, or a string keyword, from the list below.
//
//    0   input
//    1   output
//    2   inout
//    3   tristate
//    4   clock
//    5   outclock
//    6   supply
//    7   outsupply
//    9   ground
//
// The function returns one if the type is set successfully, zero
// otherwise.
//
bool
extract_funcs::IFsetTerminalType(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *termtype = 0;
    if (args[1].type == TYP_SCALAR) {
        int val;
        ARG_CHK(arg_int(args, 1, &val))
        for (FlagDef *f = TermTypes; f->name; f++) {
            if (f->value == (unsigned int)val) {
                termtype = f->name;
                break;
            }
        }
    }
    else {
        ARG_CHK(arg_string(args, 1, &termtype))
    }

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps)
                    res->content.value = EX()->setTerminalType(ps, termtype);
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term) {
                    CDp_snode *ps = term->node_prpty();
                    if (ps)
                        res->content.value = EX()->setTerminalType(ps, termtype);
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (int) GetTerminalFlags(terminal_handle)
//
// Return the flags for the terminal referenced by the handle passed
// as an argument, which can also be a handle to the corresponding
// physical terminal.  The return value is an integer with bits
// representing flags as listed in the table below.  On error, the
// return value is -1.
//
// The flag bits are as follows:
//   BYNAME     0x1
//      The terminal makes connections in the schematic by name rather
//      than by location.
//   VIRTUAL    0x2
//      No longer used, reserved.
//   FIXED      0x4
//      The physical terminal has been placed by the user, and Xic
//      should never move it.
//   SCINVIS    0x8  
//      The electrical terminal will not be shown in schematics.
//   SYINVIS    0x10
//      The electrical terminal will not be shown in the symbol..
//   UNINIT     0x100
//      The terminal is not initialized (internal).
//   LOCSET     0x200
//      The physical terminal location has been set (internal).
//   POINTS     0x400
//      Set when the terminal has multiple hot-spots.
//   NOPHYS     0x800
//      Set if the terminal has no physical implementation, such as
//      a temperature node.  Such terminals have no physical terminals.
//
bool
extract_funcs::IFgetTerminalFlags(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt)
                res->content.value = t->elt->term_flags();
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *sterm = dynamic_cast<CDsterm*>(t->elt);
                if (sterm) {
                    CDp_snode *ps = sterm->node_prpty();
                    if (ps)
                        res->content.value = ps->term_flags();
                }
                else {
                    CDcterm *cterm = dynamic_cast<CDcterm*>(t->elt);
                    if (cterm) {
                        CDp_cnode *pc = cterm->node_prpty();
                        if (pc)
                            res->content.value = pc->term_flags();
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (int) SetTerminalFlags(thandle, flags)
//
// This will set the first five flags listed for GetTerminalFlags in
// the terminal referenced by the first argument, which can also be a
// handle to the corresponding physical terminal..  All but the five
// least significant bits in the flags integer are ignored.  The bits
// that are set will set the corresponding flag in the terminal, unset
// bits are ignored.  The value one is returned on success, zero
// otherwise.
//
bool
extract_funcs::IFsetTerminalFlags(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int flags;
    ARG_CHK(arg_handle(args, 1, &flags))
    flags &= TE_IOFLAGS;

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                t->elt->set_flag(flags);
                res->content.value = 1;
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term) {
                    CDp_snode *ps = term->node_prpty();
                    if (ps) {
                        ps->set_flag(flags);
                        res->content.value = 1;
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (boolean) UnsetTerminalFlags(thandle, flags)
//
// This will unset the first five flags listed for GetTerminalFlags in
// the terminal referenced by the first argument, which can also be a
// handle to the corresponding physical terminal.  All but the five
// least significant bits in the flags integer are ignored.  The bits
// that are set will unset the corresponding flag in the terminal,
// unset bits are ignored.  The value one is returned on success, zero
// otherwise.
//
bool
extract_funcs::IFunsetTerminalFlags(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int flags;
    ARG_CHK(arg_handle(args, 1, &flags))
    flags &= TE_IOFLAGS;

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                t->elt->unset_flag(flags);
                res->content.value = 1;
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term) {
                    CDp_snode *ps = term->node_prpty();
                    if (ps) {
                        ps->unset_flag(flags);
                        res->content.value = 1;
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (boolean) GetElecTerminalLoc(thandle, index, array)
//
// This will return terminal locations in the electrical schematic, of
// the terminal referenced by the first argument.  This argument can
// also be a handle to the corresponding physical terminal. The return
// is dependent on whether the electrical cell is symbolic or not. 
// Values for x and y are returned in the array, which must have size
// two or larger.  The returned values are in fictitions "microns"
// that correspond to 1000 database units.  This is the same
// coordinate system indicated by the coordinate readout when editing
// a schematic.
//
// If the electrical cell is not symbolic, the integer index argument
// must be zero, and the terminal location in the schematic is
// returned.
//
// If the electrical cell is symbolic, there can be arbitrarily many
// "copies" of the terminal, representing multiple "hot spots" where
// the terminal can make connections.  The index argument is a 0-based
// index for these locations.  To get all of the locations, one should
// call this function repeatedly while incrementing the index from
// zero.  A return value of zero indicates that the index is out of
// range (or some error occurred).  A return value of one indicates
// success, with the array containing the location.
//
bool
extract_funcs::IFgetElecTerminalLoc(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int index;
    ARG_CHK(arg_int(args, 1, &index))
    double *vals = 0;
    ARG_CHK(arg_array(args, 2, &vals, 2))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps) {
                    CDs *cursde = CurCell(Electrical);
                    if (cursde) {
                        int x, y;
                        if (cursde->isSymbolic()) {
                            if (ps->get_pos(index, &x, &y)) {
                                vals[0] = ELEC_MICRONS(x);
                                vals[1] = ELEC_MICRONS(y);
                                res->content.value = 1;
                            }
                        }
                        else if (index == 0) {
                            ps->get_schem_pos(&x, &y);
                            vals[0] = ELEC_MICRONS(x);
                            vals[1] = ELEC_MICRONS(y);
                            res->content.value = 1;
                        }
                    }
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *sterm = dynamic_cast<CDsterm*>(t->elt);
                if (sterm) {
                    CDp_snode *ps = sterm->node_prpty();
                    if (ps) {
                        CDs *cursde = CurCell(Electrical);
                        if (cursde) {
                            int x, y;
                            if (cursde->isSymbolic()) {
                                if (ps->get_pos(index, &x, &y)) {
                                    vals[0] = ELEC_MICRONS(x);
                                    vals[1] = ELEC_MICRONS(y);
                                    res->content.value = 1;
                                }
                            }
                            else if (index == 0) {
                                ps->get_schem_pos(&x, &y);
                                vals[0] = ELEC_MICRONS(x);
                                vals[1] = ELEC_MICRONS(y);
                                res->content.value = 1;
                            }
                        }
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (int) SetElecTerminalLoc(thandle, x, y)
//
// This function specifies a location for the terminal referenced by
// the first arguemnt, for use in electrical mode.  The x and y are
// coordinates in fictitions "microns" which are 1000 database units. 
// This is the same coordinate system used in the coordinate readout
// when editing a schematic.  As for most of these functions, the
// first argument can also be a handle to the corresponding physical
// terminal.
//
// The function behaves differently depending on whether the
// electrical current cell is symbolic or not.  If the electrical
// current cell not symbolic, the passed coordinates set the terminal
// location within the schematic.  Otherwise, in symbolic mode, there
// can be arbitrarily many locations set.  The function will add the
// passed location to the list of locations for the terminal, if it is
// not already using the location.
//
// The function returns one on success, zero otherwise.
//
bool
extract_funcs::IFsetElecTerminalLoc(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, Electrical))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, Electrical))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps) {
                    Point_c pe(x, y);
                    res->content.value = EX()->setElecTerminalLoc(ps, &pe);
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term) {
                    CDp_snode *ps = term->node_prpty();
                    if (ps) {
                        Point_c pe(x, y);
                        res->content.value = EX()->setElecTerminalLoc(ps, &pe);
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (boolean) ClearElecTerminalLoc(thandle, x, y)
//
// This function applies only when the electric current cell is in
// symbolic mode.  When true, a terminal may be displayed in
// arbitrarily many locations, representing different possible
// connection points.  The x and y are coordinates in fictitions
// "microns" which are 1000 database units.  This is the same
// coordinate system used in the coordinate readout when editing a
// schematic.  If the coordinates match a hot spot of the terminal,
// that location is deleted.
//
// It is not possible to delete the last location, there is always at
// least one active location.  Calling this function when the
// electrical current cell is not symbolic has no effect.  The
// function returns one on success, zero if error.
//
// As for most of these functions, the first argument can also be a
// handle to the corresponding physical terminal.
//
bool
extract_funcs::IFclearElecTerminalLoc(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, Electrical))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, Electrical))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                Point_c pe(x, y);
                res->content.value = EX()->clearElecTerminalLoc(ps, &pe);
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term) {
                    CDp_snode *ps = term->node_prpty();
                    if (ps) {
                        Point_c pe(x, y);
                        res->content.value = EX()->clearElecTerminalLoc(ps, &pe);
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Physical Terminals (CDpin list)
//-------------------------------------------------------------------------

// (physterm_handle) ListPhysTerminals()
//
// This returns a handle to a list of physical terminal structures
// that correspond to the cell connection points, as obtained from the
// physical part of the current cell.
//
bool
extract_funcs::IFlistPhysTerminals(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (OK);
    CDp_snode *ps = (CDp_snode*)cursde->prpty(P_NODE);
    tlist<CDterm> *t0 = 0, *te = 0;
    for ( ; ps; ps = ps->next()) {
        if (!ps->cell_terminal())
            continue;
        if (!t0)
            t0 = te = new tlist<CDterm >(ps->cell_terminal(), 0);
        else {
            te->next = new tlist<CDterm>(ps->cell_terminal(), 0);
            te = te->next;
        }
    }
    if (t0) {
        sHdl *nhdl = new sHdlTerminal(t0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    return (OK);
}


// (physterm_handle) FindPhysTerminal(name, use_p, xp, yp)
//
// This attempts to find a physical terminal structure by name or
// location.  If a name is given, i.e., the argument is not null or 0,
// then it will match the name of the terminal returned.  If the
// boolean use_p is nonzero (true), then the coordinates xp and yp,
// given in microns, will match the placement location of the returned
// terminal.  If both name and coordinates are given, both must match.
//
// An empty handle (scalar 0) is returned if there is no matching
// physical terminal found.
//
bool
extract_funcs::IFfindPhysTerminal(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    bool use_p;
    ARG_CHK(arg_boolean(args, 1, &use_p))
    int xp;
    ARG_CHK(arg_coord(args, 2, &xp, Physical))
    int yp;
    ARG_CHK(arg_coord(args, 3, &yp, Physical))

    CDs *psd = CurCell(Physical);
    if (!psd)
        return (BAD);

    CDnetName name_stab = CDnetex::name_tab_find(name);
    tlist<CDterm> *t0 = 0;
    for (CDpin  *p = psd->pins(); p; p = p->next()) {
        if (name_stab && p->term()->name() != name_stab)
            continue;
        if (use_p && (xp != p->term()->lx() || yp != p->term()->ly()))
            continue;
        t0 = new tlist<CDterm>(p->term(), 0);
    }
    if (t0) {
        sHdl *nhdl = new sHdlTerminal(t0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    else {
        res->type = TYP_SCALAR;
        res->content.value = 0;
    }
    return (OK);
}


// (int) CreatePhysTerminal(thandle, x, y, layer)
//
// As created, (electrical) terminals do not contain the data
// structures necessary for a corresponding terminal in the physical
// layout.  This is fine as-is, if the user is intending to only work
// with a schematic, or if the terminal does not have an actual
// physical counterpart.  However, in general one must create the
// physical terminal.
//
// This function will create a new physical terminal, if one of the
// same name does not currently exist.  The first argument can be a
// handle to a terminal (electrical node) or a string giving a name. 
// In the first case, the new physical terminal is created, given the
// name of the electrical terminal, and the linkage established.  In
// the second case, which does not require the existance of the
// electrical schematic, the physical terminal is created under the
// given name, and saved in the physical data.  It will be linked to
// corresponding electrical data during association, when possible.
//
// The x and y give the initial terminal location in the layout in
// microns.  The layer argument can be scalar 0, which is ignored, or
// the name of a layer.  The layer must have the ROUTING keyword
// applied.  If given, this will set the layer hint for the new
// terminal.
//
// The return value is 1 on success, 0 otherwise.  It is not an error
// if the physical terminal already exists, the function will return 1
// and perform no other operation in that case.
//
bool
extract_funcs::IFcreatePhysTerminal(Variable *res, Variable *args, void*)
{
    const char *name = 0;
    int id = -1;
    if (args[0].type == TYP_STRING) {
        ARG_CHK(arg_string(args, 0, &name))
    }
    else if (args[0].type == TYP_HANDLE || args[0].type == TYP_SCALAR) {
        ARG_CHK(arg_handle(args, 0, &id))
    }
    int x;
    ARG_CHK(arg_coord(args, 1, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, Physical))
    const char *lname;
    ARG_CHK(arg_string(args, 3, &lname))

    res->type = TYP_SCALAR;
    res->content.value = 0;

    sHdl *hdl = sHdl::get(id);
    if (hdl) {
        if (hdl->type != HDLnode)
            return (BAD);
        tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
        if (t && t->elt) {
            CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
            if (ps)
                name = Tstring(ps->term_name());
        }
    }
    else if (name) {
        Point_c pp(x, y);
        res->content.value = EX()->createPhysTerminal(CurCell(Physical),
            name, &pp, lname);
    }
    return (OK);
}


// (int) HasPhysTerminal(thandle)
//
// This function returns 1 if the terminal referenced by the handle
// argument has a physical terminal link, 0 if no link has been
// assigned.  On error, a value -1 is returned.
//
bool
extract_funcs::IFhasPhysTerminal(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLnode)
            return (BAD);
        tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
        if (t && t->elt) {
            CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
            if (ps)
                res->content.value = (ps->cell_terminal() != 0);
        }
    }
    return (OK);
}


// (boolean) DestroyPhysTerminal(thandle)
//
// This will unlink and destroy the physical terminal data structure
// that maintains the terminal linkage into the physical layout, if
// any.  The argument can be a handle to the corresponding electrical
// terminal, or to the physical terminal itself.  In the latter case,
// the passed handle will be closed.  The electrical terminal (if any)
// will still be valid, as will its handle if that was passed.  The
// function returns one on success, zero if an error occurs.
//
bool
extract_funcs::IFdestroyPhysTerminal(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps) {
                    res->content.value = EX()->destroyPhysTerminal(
                        ps->cell_terminal());
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term)
                    res->content.value = EX()->destroyPhysTerminal(term);
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (int) GetPhysTerminalLoc(terminal_handle, array)
//
// Return the layout location for the physical terminal referenced by
// the handle passed as an argument.  The first argument can
// alternatively be a handle to the corresponding electrical terminal. 
// The second argument is an array of size two or larger which will
// receive the x-y coordinate, in microns.  The function returns one
// on success, zero otherwise.
//
bool
extract_funcs::IFgetPhysTerminalLoc(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals = 0;
    ARG_CHK(arg_array(args, 1, &vals, 2))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps && ps->cell_terminal()) {
                    int x = ps->cell_terminal()->lx();
                    int y = ps->cell_terminal()->ly();
                    vals[0] = MICRONS(x);
                    vals[1] = MICRONS(y);
                    res->content.value = 1;
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *sterm = dynamic_cast<CDsterm*>(t->elt);
                if (sterm) {
                    int x = sterm->lx();
                    int y = sterm->ly();
                    vals[0] = MICRONS(x);
                    vals[1] = MICRONS(y);
                    res->content.value = 1;
                }
                else {
                    CDcterm *cterm = dynamic_cast<CDcterm*>(t->elt);
                    if (cterm) {
                        int x = cterm->lx();
                        int y = cterm->ly();
                        vals[0] = MICRONS(x);
                        vals[1] = MICRONS(y);
                        res->content.value = 1;
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (boolean) SetPhysTerminalLoc(thandle, x, y)
//
// Set the location of the physical terminal referenced by the first
// argument to the layout coordinate given, in microns.  The first
// argument can also be a handle to the corresponding electrical
// terminal.  Generally, physical terminal locations are set by Xic,
// using extraction results.  However, this may fail, requiring that
// the user provide a location for one or more terminals.  Terminals
// that have been placed by the user (using this function) will by
// default remain fixed in the location.  The function returns one on
// success, zero if an error occurs.
//
bool
extract_funcs::IFsetPhysTerminalLoc(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int x;
    ARG_CHK(arg_coord(args, 1, &x, Physical))
    int y;
    ARG_CHK(arg_coord(args, 2, &y, Physical))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps && ps->cell_terminal()) {
                    CDsterm *term = ps->cell_terminal();
                    if (term) {
                        Point_c pp(x, y);
                        bool ok = EX()->setPhysTerminalLoc(term, &pp);
                        if (ok)
                            ps->set_flag(TE_FIXED);
                        res->content.value = ok;
                    }
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term) {
                    Point_c pp(x, y);
                    bool ok = EX()->setPhysTerminalLoc(term, &pp);
                    if (ok) {
                        CDp_snode *ps = term->node_prpty();
                        if (ps)
                            ps->set_flag(TE_FIXED);
                    }
                    res->content.value = ok;
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (string) GetPhysTerminalLayer(terminal_handle)
//
// Return a string containing the layer name for the physical terminal
// referenced by the handle passed as an argument.  A handle to the
// corresponding electrical terminal is also accepted.  Non-virtual
// physical terminals are associated with an object on a ROUTING
// layer.  A null string is returned if there is no associated layer.
//
bool
extract_funcs::IFgetPhysTerminalLayer(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps && ps->cell_terminal()) {
                    CDl *ld = ps->cell_terminal()->layer();
                    if (ld) {
                        res->content.string = lstring::copy(ld->name());
                        res->flags |= VF_ORIGINAL;
                    }
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *sterm = dynamic_cast<CDsterm*>(t->elt);
                if (sterm) {
                    CDl *ld = sterm->layer();
                    if (ld) {
                        res->content.string = lstring::copy(ld->name());
                        res->flags |= VF_ORIGINAL;
                    }
                }
                else {
                    CDcterm *cterm = dynamic_cast<CDcterm*>(t->elt);
                    if (cterm) {
                        CDl *ld = cterm->layer();
                        if (ld) {
                            res->content.string = lstring::copy(ld->name());
                            res->flags |= VF_ORIGINAL;
                        }
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (boolean) SetPhysTerminalLayer(thandle, lname)
//
// This function will set the associated layer hint on the physical
// terminal referenced by the handle passed as the first argument.  A
// handle to the corresponding electrical terminal is also accepted. 
// If the second argument is the name of a physical layer which has
// the ROUTING keyword set, the terminal hint layer will be set to
// that layer.  If the second argument is a scalar 0, or a null or
// empty string, any existing hint layer will be removed.  The
// function returns one on success, zero otherwise.
//
bool
extract_funcs::IFsetPhysTerminalLayer(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *lname;
    ARG_CHK(arg_string(args, 1, &lname))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps) {
                    res->content.value =
                        EX()->setTerminalLayer(ps->cell_terminal(), lname);
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *term = dynamic_cast<CDsterm*>(t->elt);
                if (term)
                    res->content.value = EX()->setTerminalLayer(term, lname);
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (int) GetPhysTerminalGroup(thandle)
//
// This function will return the conductor group number to which the
// physical terminal referenced by the argument is assigned.  A handle
// to the corresponding electrical terminal is also accepted.  The
// group assignment is made during extraction and association.  The
// return value is a non-negative integer on success, or -1 if
// extraction/association has not been run (or been reverted), or -2
// if some error occurred.
//
bool
extract_funcs::IFgetPhysTerminalGroup(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -2;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps && ps->cell_terminal())
                    res->content.value = ps->cell_terminal()->group();
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *sterm = dynamic_cast<CDsterm*>(t->elt);
                if (sterm)
                    res->content.value = sterm->group();
                else {
                    CDcterm *cterm = dynamic_cast<CDcterm*>(t->elt);
                    if (cterm)
                        res->content.value = cterm->group();
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (object_handle) GetPhysTerminalObject(thandle)
//
// Return a handle to a physical object that is associated with the
// physical terminal referenced by the handle passed as an argument. 
// A handle to the coresponding electrical terminal is also accepted. 
// Physical terminals are associated with underlying conducting
// objects as part of the connectivity algorithm.  Not all terminals
// have an associated object, in which case they are "virtual".  An
// empty handle (scalar 0) is returned in this case.
//
bool
extract_funcs::IFgetPhysTerminalObject(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_snode *ps = dynamic_cast<CDp_snode*>(t->elt);
                if (ps && ps->cell_terminal()) {
                    CDo *oset = ps->cell_terminal()->get_ref();
                    if (oset) {
                        CDol *ol = new CDol(oset, 0);
                        sHdl *nhdl = new sHdlObject(ol, cursdp, false);
                        res->type = TYP_HANDLE;
                        res->content.value = nhdl->id;
                    }
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDsterm *sterm = dynamic_cast<CDsterm*>(t->elt);
                if (sterm) {
                    CDo *oset = sterm->get_ref();
                    if (oset) {
                        CDol *ol = new CDol(oset, 0);
                        sHdl *nhdl = new sHdlObject(ol, cursdp, false);
                        res->type = TYP_HANDLE;
                        res->content.value = nhdl->id;
                    }
                }
                else {
                    CDcterm *cterm = dynamic_cast<CDcterm*>(t->elt);
                    if (cterm) {
                        CDo *oset = cterm->get_ref();
                        if (oset) {
                            CDol *ol = new CDol(oset, 0);
                            sHdl *nhdl = new sHdlObject(ol, cursdp, false);
                            res->type = TYP_HANDLE;
                            res->content.value = nhdl->id;
                        }
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Physical Conductor Groups
//-------------------------------------------------------------------------

// (int) Group()
//
// This function will run the grouping and device extraction algorithm
// on the current physical cell.  The grouping algorithm identifies
// the wire nets.  The returned value is the number of groups used, or
// 0 if an error occurs.  The group index extends from 0 through the
// number returned minus one.  Group 0 is the ground group, if a
// ground plane layer has been defined.
//
bool
extract_funcs::IFgroup(Variable *res, Variable*, void*)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd)
            res->content.value = gd->nextindex();
    }
    return (OK);
}


// (int) GetNumberGroups()
//
// Return the number of conductor groups allocated by the extraction
// process in the physical part of the current cell.  The group index
// passed to other functions should be less than this value.
//
bool
extract_funcs::IFgetNumberGroups(Variable *res, Variable*, void*)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd)
            res->content.value = gd->num_groups();
    }
    return (OK);
}


// (int) GetGroupBB(group, array)
//
// This function returns the bounding box of the conductor group whose
// index is passed as the first argument.  The coordinates, in microns
// relative the the current physical cell origin, are returned in the
// array, which must have size 4 or larger.  If the function succeeds,
// 1 is returned, otherwise 0 is returned.  The saved order is L, B,
// R, T.
//
bool
extract_funcs::IFgetGroupBB(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))
    double *vals = 0;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g && g->net()) {
                vals[0] = MICRONS(g->net()->BB().left);
                vals[1] = MICRONS(g->net()->BB().bottom);
                vals[2] = MICRONS(g->net()->BB().right);
                vals[3] = MICRONS(g->net()->BB().top);
                res->content.value = 1;
            }
        }
    }
    return (OK);
}


// (int) GetGroupNode(group)
//
// This function returns the node number from the electrical database
// which corresponds to the physical group index passed as the
// argument.  If the association failed, -1 is returned.
//
bool
extract_funcs::IFgetGroupNode(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g)
                res->content.value = g->node();
        }
    }
    return (OK);
}


// (string) GetGroupName(group)
//
// Return a string containing a name for the group whose number is
// passed as the argument.  The name is the name of a formal terminal
// attached to the group, or the net name if no formal terminal.  If
// the group has no name, a null string is returned.
//
bool
extract_funcs::IFgetGroupName(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            bool ft, cf;
            const char *nm = gd->group_name(group, &ft, &cf);
            if (nm) {
                res->content.string = lstring::copy(nm);
                res->flags |= VF_ORIGINAL;
            }
        }
    }
    return (OK);
}


// (string) GetGroupNetName(group)
//
// Return a string containing the net name for the group whose number
// is passed as the argument.  If the group has no net name, a null
// string is returned.
//
bool
extract_funcs::IFgetGroupNetName(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g) {
                res->content.string = lstring::copy(Tstring(g->netname()));
                res->flags |= VF_ORIGINAL;
            }
        }
    }
    return (OK);
}


// (real) GetGroupCapacitance(group)
//
// Return the capacitance assigned to the group whose index is passed
// as the argument.  If no capacitance has been assigned.  0 is
// returned.
//
bool
extract_funcs::IFgetGroupCapacitance(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g)
                res->content.value = g->capac();
        }
    }
    return (OK);
}


// (int) CountGroupObjects(group)
//
// Return the number of physical objects that implement the group.  If
// there is an error, such as the argument being out of range, -1 is
// returned.
//
bool
extract_funcs::IFcountGroupObjects(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            int cnt = 0;
            sGroupObjs *go = gd->net_of_group(group);
            if (go) {
                for (CDol *o = go->objlist(); o; o = o->next)
                    cnt++;
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (object_handle) ListGroupObjects(group)
//
// This function returns a handle to the list of objects associated
// with the current physical cell which constitute the group, as found
// by the extraction system.  These may or may not correspond to
// actual objects in the cell.  For example, the objects returned have
// been processed by the "Conductor Exclude" directive, so would
// possibly be clipped versions of the original objects. 
// Additionally, objects from wire-only subcells and vias that have
// been logically flattened during extraction will be included. 
// Objects from flattened via instances will have the MergeCreated
// (0x1) flag set, which can be tested with GetObjectFlags.  This
// allows the caller to filter out redundant metal if standard vias
// are used, in addition to the objects, to represent the net.
//
// The argument is the group number.  The returned objects are copies,
// so can not be modified or selected.  If an error occurs, 0 is
// returned.
//
bool
extract_funcs::IFlistGroupObjects(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroupObjs *go = gd->net_of_group(group);
            if (go) {
                CDol *ol0 = 0, *ole = 0;
                for (CDol *o = go->objlist(); o; o = o->next) {
                    CDo *od = o->odesc->copyObject();
                    if (!ol0)
                        ol0 = ole = new CDol(od, 0);
                    else {
                        ole->next = new CDol(od, 0);
                        ole = ole->next;
                    }
                }
                sHdl *hdl = new sHdlObject(ol0, cursdp, true);
                res->type = TYP_HANDLE;
                res->content.value = hdl->id;
            }
        }
    }
    return (OK);
}


// (int) CountGroupVias(group)
//
// Return the number of via instances used to implement the group,
// from the extraction system.  This is the number of vias that would
// be returned by ListGroupVias (below).  If there is an error, such
// as the group number argument being out of range, -1 is returned.
//
bool
extract_funcs::IFcountGroupVias(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            int cnt = 0;
            sGroupObjs *go = gd->net_of_group(group);
            if (go) {
                for (CDol *c = go->vialist(); c; c = c->next)
                    cnt++;
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (object_handle) ListGroupVias(group)
//
// This function returns a handle to the list of via instances
// associated with the current physical cell which are used in the
// group, as obtained from the extraction system.  This may include
// vias that were "promoted" due to the logical flattening of
// wire-only subcells during extraction.  Vias in such cells are
// treated as if they reside in their parent cells, recursively.
//
// The argument is the group number.  The via instances are copies, so
// can not be modified or selected.  If an error occurs, 0 is
// returned.
//
bool
extract_funcs::IFlistGroupVias(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroupObjs *go = gd->net_of_group(group);
            if (go) {
                CDol *ol0 = 0, *ole = 0;
                for (CDol *c = go->vialist(); c; c = c->next) {
                    CDo *od = c->odesc;
                    if (!ol0)
                        ol0 = ole = new CDol(od, 0);
                    else {
                        ole->next = new CDol(od, 0);
                        ole = ole->next;
                    }
                }
                sHdl *hdl = new sHdlObject(ol0, cursdp);
                res->type = TYP_HANDLE;
                res->content.value = hdl->id;
            }
        }
    }
    return (OK);
}


// (int) CountGroupDevContacts(group)
//
// This function returns a count of the number of device contacts
// which are assigned to the conductor group whose index is passed as
// the argument.  If an error occurs, -1 is returned.
//
bool
extract_funcs::IFcountGroupDevContacts(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            int cnt = 0;
            sGroup *g = gd->group_for(group);
            if (g) {
                for (sDevContactList *dc = g->device_contacts(); dc;
                        dc = dc->next()) {
                    cnt++;
                }
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (dev_contact_handle) ListGroupDevContacts(group)
//
// This function returns a handle to the list of device contacts which
// are assigned to the conductor group whose index is passed as the
// argument.  If an error occurs, 0 is returned.
//
bool
extract_funcs::IFlistGroupDevContacts(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g) {
                tlist<sDevContactInst> *ct0 = 0, *cte = 0;
                for (sDevContactList *dc = g->device_contacts(); dc;
                        dc = dc->next()) {
                    if (!ct0)
                        ct0 = cte =
                            new tlist<sDevContactInst>(dc->contact(), 0);
                    else {
                        cte->next =
                            new tlist<sDevContactInst>(dc->contact(), 0);
                        cte = cte->next;
                    }
                }
                sHdl *nhdl = new sHdlDevContact(ct0, gd);
                res->type = TYP_HANDLE;
                res->content.value = nhdl->id;
            }
        }
    }
    return (OK);
}


// (int) CounttGroupSubcContacts(group)
//
// This function returns a count of subcircuit contacts associated
// with the group index passed as the argument.  If an error occurs,
// -1 is returned.
//
bool
extract_funcs::IFcountGroupSubcContacts(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            int cnt = 0;
            sGroup *g = gd->group_for(group);
            if (g) {
                for (sSubcContactList *sc = g->subc_contacts(); sc;
                        sc = sc->next()) {
                    cnt++;
                }
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (subc_contact_handle) ListGroupSubcContacts(group)
//
// This function returns a handle to a list of subcircuit contacts
// associated with the group index passed as the argument.  If an
// error occurs, 0 is returned.
//
bool
extract_funcs::IFlistGroupSubcContacts(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g) {
                tlist<sSubcContactInst> *st0 = 0, *ste = 0;
                for (sSubcContactList *sc = g->subc_contacts(); sc;
                        sc = sc->next()) {
                    if (!st0)
                        st0 = ste =
                            new tlist<sSubcContactInst>(sc->contact(), 0);
                    else {
                        ste->next =
                            new tlist<sSubcContactInst>(sc->contact(), 0);
                        ste = ste->next;
                    }
                }
                sHdl *nhdl = new sHdlSubcContact(st0, gd);
                res->type = TYP_HANDLE;
                res->content.value = nhdl->id;
            }
        }
    }
    return (OK);
}


// (int) CountGroupTerminals(group)
//
// Return a count of cell connection terminals associated with the
// group number passed as an argument.  If an error occurs, -1 is
// returned.
//
bool
extract_funcs::IFcountGroupTerminals(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            int cnt = 0;
            sGroup *g = gd->group_for(group);
            if (g) {
                for (CDpin *p = g->termlist(); p; p = p->next()) {
                    if (p->term()->node_prpty())
                        cnt++;
                }
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (terminal_handle) ListGroupTerminals(group)
//
// Return a handle to a list of cell connection terminals associated
// with the group number passed as an argument.  If an error occurs, 0
// is returned.  If the group contains no cell connection terminals,
// the list will be empty.
//
bool
extract_funcs::IFlistGroupTerminals(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDcbin cbin(CurCell(Physical));
    if (!cbin.phys())
        return (BAD);
    
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cbin.phys())) {
        cGroupDesc *gd = cbin.phys()->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g) {
                tlist2<CDp_nodeEx> *t0 = 0, *te = 0;
                for (CDpin *p = g->termlist(); p; p = p->next()) {
                    CDp_snode *ps = p->term()->node_prpty();
                    if (ps) {
                        if (!t0)
                            t0 = te = new tlist2<CDp_nodeEx>(ps, cbin.elec(), 0);
                        else {
                            te->next = new tlist2<CDp_nodeEx>(ps, cbin.elec(), 0);
                            te = te->next;
                        }
                    }
                }
                sHdl *nhdl = new sHdlNode(t0);
                res->type = TYP_HANDLE;
                res->content.value = nhdl->id;
            }
        }
    }
    return (OK);
}


// (int) CountGroupPhysTerminals(group)
//
// Return a count of the physical terminal descriptors from the
// physical cell that are associated with the group number given.
//
bool
extract_funcs::IFcountGroupPhysTerminals(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            int cnt = 0;
            sGroup *g = gd->group_for(group);
            if (g) {
                for (CDpin *p = g->termlist(); p; p = p->next())
                    cnt++;
            }
            res->content.value = cnt;
        }
    }
    return (OK);
}


// (physterm_handle) ListGroupPhysTerminals(group)
//
// Return a handle to a list of the physical terminal descriptors from the
// physical cell that are associated with the group number given.
//
bool
extract_funcs::IFlistGroupPhysTerminals(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g) {
                tlist<CDterm> *t0 = 0, *te = 0;
                for (CDpin *p = g->termlist(); p; p = p->next()) {
                    if (!t0)
                        t0 = te = new tlist<CDterm >(p->term(), 0);
                    else {
                        te->next = new tlist<CDterm>(p->term(), 0);
                        te = te->next;
                    }
                }
                sHdl *nhdl = new sHdlTerminal(t0);
                res->type = TYP_HANDLE;
                res->content.value = nhdl->id;
            }
        }
    }
    return (OK);
}


// (stringlist_handle) ListGroupTerminalNames(group)
//
// This function returns a list of names of the cell connection
// terminals assigned to the conductor group whose index is passed as
// the argument.  If an error occurs, 0 is returned.  If the group
// contains no cell connection terminals, the list will be empty.
//
bool
extract_funcs::IFlistGroupTerminalNames(Variable *res, Variable *args, void*)
{
    int group;
    ARG_CHK(arg_int(args, 0, &group))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->extract(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            sGroup *g = gd->group_for(group);
            if (g) {
                stringlist *s0 = 0;
                for (CDpin *p = g->termlist(); p; p = p->next()) {
                    s0 = new stringlist(
                        lstring::copy(Tstring(p->term()->name())), s0);
                }
                sHdl *nhdl = new sHdlString(s0);
                res->type = TYP_HANDLE;
                res->content.value = nhdl->id;
            }
        }
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Physical Devices
//-------------------------------------------------------------------------

// (device_handle) ListPhysDevs(name, pref, indices, area_array)
//
// This function returns a handle to a list of devices extracted from
// the physical part of the current cell.  The first two arguments are
// strings which match the Name and Prefix fields from the technology
// file Device block of the device to list.  Either or both of
// these arguments can be null or empty, in which case no devices are
// excluded by the comparison, i.e., such values act as wildcards.
//
// The third argument is a string providing a list of device indices,
// or ranges of indices, to allow.  These are integers that are unique
// to each instance of a device type in a cell.  If this argument is
// null or empty, all indices will be returned.  Each token in the
// string is an integer (e.g., "2"), or range of integers (e.g.,
// "1-4"), using the hyphen (minus sign) to separate the minimum and
// maximum index to include.  The tokens are separated by white space
// and/or commas.  For example, "1,3-5,7,9-12".
//
// The final argument, if not 0, is an array of size four or larger
// containing rectangle coordinates, in microns, in order L,B,R,T.  If
// 0 is passed for this argument, the entire cell is searched for
// devices.  Otherwise, only the area provided will be searched.
//
// On success, a handle is returned, otherwise 0 is returned.  The
// handle can be used in the functions that take a device handle as an
// argument.  This is *not* an object handle.  The returned
// device_handle can be manipulated with the generic handle functions,
// and like other handles should be iterated through or explicitly
// closed when no longer needed.
//
bool
extract_funcs::IFlistPhysDevs(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    const char *pref;
    ARG_CHK(arg_string(args, 1, &pref))
    const char *inds;
    ARG_CHK(arg_string(args, 2, &inds))
    double *area;
    ARG_CHK(arg_array_if(args, 3, &area, 4))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->associate(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            BBox BB;
            if (area) {
                BB.left = INTERNAL_UNITS(area[0]);
                BB.bottom = INTERNAL_UNITS(area[1]);
                BB.right = INTERNAL_UNITS(area[2]);
                BB.top = INTERNAL_UNITS(area[3]);
                BB.fix();
            }
            sDevInstList *dv0 = gd->find_dev(name, pref, inds, area ? &BB : 0);
            tlist<sDevInst> *t0 = 0, *te = 0;
            for (sDevInstList *dv = dv0; dv; dv = dv->next) {
                if (!t0)
                    t0 = te = new tlist<sDevInst>(dv->dev, 0);
                else {
                    te->next = new tlist<sDevInst>(dv->dev, 0);
                    te = te->next;
                }
            }
            sDevInstList::destroy(dv0);
            sHdl *hdl = new sHdlDevice(t0, gd);
            res->type = TYP_HANDLE;
            res->content.value = hdl->id;
        }
    }
    return (OK);
}


// (string) GetPdevName(device_handle)
//
// This function returns a string containing the name of the device
// referenced by the handle.  The name string is composed of the Name
// field for the device (from the Device block), followed by an
// underscore, followed by the device index number.  If the handle is
// defunct or some other error occurs, a null string is returned.
//
bool
extract_funcs::IFgetPdevName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLdevice)
            return (BAD);
        tlist<sDevInst> *t = (tlist<sDevInst>*)hdl->data;
        if (t) {
            char buf[128];
            char *s = lstring::stpcpy(buf, TstringNN(t->elt->desc()->name()));
            *s++ = '_';
            mmItoA(s, t->elt->index());
            res->content.string = lstring::copy(buf);
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) GetPdevIndex(device_handle)
//
// This function returns the index of the device referenced by the
// handle passed as an argument.  The index is an integer which is
// unique among the devices of a given type.  If the handle is defunct
// or an error occurs, -1 is returned.
//
bool
extract_funcs::IFgetPdevIndex(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLdevice)
            return (BAD);
        tlist<sDevInst> *t = (tlist<sDevInst>*)hdl->data;
        if (t)
            res->content.value = t->elt->index();
    }
    return (OK);
}


// (object_handle) GetPdevDual(device_handle)
//
// The function returns an object_handle which references the dual
// device in the electrical database to the physical device referenced
// by the argument.  If association failed for the device, 0 is
// returned.  The dual device is a subcell obtained from the device
// library.
//
bool
extract_funcs::IFgetPdevDual(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLdevice)
            return (BAD);
        tlist<sDevInst> *t = (tlist<sDevInst>*)hdl->data;
        if (t && t->elt->dual()) {
            CDol *ol0 = new CDol(t->elt->dual()->cdesc(), 0);
            sHdl *nhdl = new sHdlObject(ol0, cursde);
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (int) GetPdevBB(device_handle, array)
//
// This function obtains the bounding box of the device referenced by
// the first argument.  The coordinates, in microns using the origin
// of the current physical cell, are returned in the array, which must
// have size 4 or larger.  If the function succeeds, 1 is returned,
// otherwise the returned value is 0.  The saved order is L, B, R, T.
//
bool
extract_funcs::IFgetPdevBB(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLdevice)
            return (BAD);
        tlist<sDevInst> *t = (tlist<sDevInst>*)hdl->data;
        if (t) {
            vals[0] = MICRONS(t->elt->BB()->left);
            vals[1] = MICRONS(t->elt->BB()->bottom);
            vals[2] = MICRONS(t->elt->BB()->right);
            vals[3] = MICRONS(t->elt->BB()->top);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (real) GetPdevMeasure(device_handle, mname)
//
// This function returns a device parameter corresponding to a Measure
// line given in the device block for the device referenced by the
// first argument.  The second argument is a string giving the name
// from a Measure line.  The returned value is the measured parameter,
// or 0 if there was an error.
//
bool
extract_funcs::IFgetPdevMeasure(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    const char *mname;
    ARG_CHK(arg_string(args, 1, &mname))

    if (!mname)
        return (BAD);
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLdevice)
            return (BAD);
        tlist<sDevInst> *t = (tlist<sDevInst>*)hdl->data;
        if (t) {
            sMeasure *m = t->elt->desc()->find_measure(mname);
            if (m) {
                t->elt->measure();
                m->measure();
                res->content.value = m->result()->content.value;
            }
        }
    }
    return (OK);
}


// (stringlist_handle) ListPdevMeasures(device_handle)
//
// This function returns a string list handle corresponding to a list
// of the names associated with Measure lines in the Device block for
// the device referenced by the handle.  These are the names that can
// be passed to GetDevMeasure() to perform the measurement.  If an
// error occurs, 0 is returned.
//
bool
extract_funcs::IFlistPdevMeasures(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLdevice)
            return (BAD);
        tlist<sDevInst> *t = (tlist<sDevInst>*)hdl->data;
        if (t) {
            stringlist *s0 = 0;
            for (sMeasure *m = t->elt->desc()->measures(); m; m = m->next())
                s0 = new stringlist(lstring::copy(m->name()), s0);
            stringlist::reverse(s0);
            sHdl *nhdl = new sHdlString(s0);
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (dev_contact_handle) ListPdevContacts(device_handle)
//
// This function returns a handle to a list of contact descriptors for
// the device referenced by the argument.  The returned handle can be
// passed to the functions below to obtain information about the
// device contacts.  If there is an error, 0 is returned.  The
// returned handle can be manipulated with the generic handle
// functions, and like other handles should be iterated through or
// closed explicitly when no longer needed.
//
bool
extract_funcs::IFlistPdevContacts(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLdevice)
            return (BAD);
        tlist<sDevInst> *t = (tlist<sDevInst>*)hdl->data;
        if (t) {
            tlist<sDevContactInst> *ct0 = 0, *cte = 0;
            for (sDevContactInst *c = t->elt->contacts(); c; c = c->next()) {
                if (!ct0)
                    ct0 = cte = new tlist<sDevContactInst>(c, 0);
                else {
                    cte->next = new tlist<sDevContactInst>(c, 0);
                    cte = cte->next;
                }
            }
            sHdl *nhdl = new sHdlDevContact(ct0, ((sHdlDevice*)hdl)->gdesc);
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (string) GetPdevContactName(dev_contact_handle)
//
// This function returns the name string of the contact referenced by
// the argument.  Contact names are assigned in the Device block for
// the device containing the contact.  If an error occurs, a null
// string is returned.
//
bool
extract_funcs::IFgetPdevContactName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLdcontact)
            return (BAD);
        tlist<sDevContactInst> *ct = (tlist<sDevContactInst>*)hdl->data;
        if (ct) {
            res->content.string =
                lstring::copy(TstringNN(ct->elt->desc()->name()));
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) GetPdevContactBB(dev_contact_handle, array)
//
// This function returns the bounding box of the contact referenced by
// the first argument.  The coordinates, in microns relative to the
// origin of the physical current cell, are returned in the array,
// which numst have size 4 or larger.  If the operation is successful,
// 1 is returned, otherwise 0 is returned.
//
bool
extract_funcs::IFgetPdevContactBB(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLdcontact)
            return (BAD);
        tlist<sDevContactInst> *ct = (tlist<sDevContactInst>*)hdl->data;
        if (ct) {
            vals[0] = MICRONS(ct->elt->cBB()->left);
            vals[1] = MICRONS(ct->elt->cBB()->bottom);
            vals[2] = MICRONS(ct->elt->cBB()->right);
            vals[3] = MICRONS(ct->elt->cBB()->top);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GetPdevContactGroup(dev_contact_handle)
//
// This function returns the conductor group index to which the
// contact referenced by the argument is assigned.  If there is an
// error, -1 is returned.
//
bool
extract_funcs::IFgetPdevContactGroup(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLdcontact)
            return (BAD);
        tlist<sDevContactInst> *ct = (tlist<sDevContactInst>*)hdl->data;
        if (ct)
            res->content.value = ct->elt->group();
    }
    return (OK);
}


// (string) GetPdevContactLayer(dev_contact_handle)
//
// This furntion returns the name string of the layer to which the
// contact referenced by the argument is assigned.  All contacts are
// assigned to layers which have the Conductor attribute.  If there is
// an error, a null string is returned.
//
bool
extract_funcs::IFgetPdevContactLayer(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLdcontact)
            return (BAD);
        tlist<sDevContactInst> *ct = (tlist<sDevContactInst>*)hdl->data;
        if (ct)
            res->content.string = lstring::copy(ct->elt->desc()->lname());
    }
    return (OK);
}


// (device_handle) GetPdevContactDev(dev_contact_handle)
//
// This function returns a handle to the device containing the contact
// referenced by the argument.  If an error occurs, 0 is returned.
// The returned handle should be closed (for example, with the Close()
// function) when no longer needed.
//
bool
extract_funcs::IFgetPdevContactDev(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLdcontact)
            return (BAD);
        tlist<sDevContactInst> *ct = (tlist<sDevContactInst>*)hdl->data;
        if (ct) {
            tlist<sDevInst> *dv = new tlist<sDevInst>(ct->elt->dev(), 0);
            sHdl *nhdl = new sHdlDevice(dv, ((sHdlDevContact*)hdl)->gdesc);
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (string) GetPdevContactDevName(dev_contact_handle)
//
// This function returns the name of the device containing the contact
// referenced by the argument.  A null string is returned on error.

bool
extract_funcs::IFgetPdevContactDevName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLdcontact)
            return (BAD);
        tlist<sDevContactInst> *ct = (tlist<sDevContactInst>*)hdl->data;
        if (ct && ct->elt->dev() && ct->elt->dev()->desc()) {
            res->content.string =
                lstring::copy(TstringNN(ct->elt->dev()->desc()->name()));
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) GetPdevContactDevIndex(dev_contact_handle)
//
// This returns the index number of the device to which the contact,
// referenced by the passed handle, is associated.  Each device of a
// given type has an index number assigned, which is unique in the
// containing cell.  On error, -1 is returned.  A valid index is 0 or
// larger.
//
bool
extract_funcs::IFgetPdevContactDevIndex(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLdcontact)
            return (BAD);
        tlist<sDevContactInst> *ct = (tlist<sDevContactInst>*)hdl->data;
        if (ct && ct->elt->dev())
            res->content.value = ct->elt->dev()->index();
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Physical Subcircuits
//-------------------------------------------------------------------------

// (subckt_handle) ListPhysSubckts(name, index, l, b, r, t)
//
// This function returns a handle to a list of subcircuits from the
// physical part of the current cell.  Subcircuits are subcells which
// contain devices or sub-subcells that contain devices.  Subcells
// that contain only wire are typically not saved internally as
// subcircuits.  The first argument is a string name which will match
// the returned subcircuits.  If this argument is null or empty, then
// this test will not exclude any subcircuits to be returned.  The
// second argument is the index number of the subcircuit to be
// returned.  If the value is -1, subcells with any index will be
// returned.  The remaining four values define a rectangular area,
// given in microns relative the the current physical cell origin,
// where subcircuits will be searched for.  If all four values are 0,
// the entire cell will be searched.  The returned handle references
// subcircuits, and is distinct from device handles and object
// handles.  The handle can be passed to the generic handle functions,
// and like other handles should be iterated through or closed when no
// longer needed.  The function returns 0 if an error occurs.
//
bool
extract_funcs::IFlistPhysSubckts(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))
    int indx;
    ARG_CHK(arg_int(args, 1, &indx))
    BBox AOI;
    ARG_CHK(arg_coord(args, 2, &AOI.left, Physical))
    ARG_CHK(arg_coord(args, 3, &AOI.bottom, Physical))
    ARG_CHK(arg_coord(args, 4, &AOI.right, Physical))
    ARG_CHK(arg_coord(args, 5, &AOI.top, Physical))
    AOI.fix();

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (EX()->associate(cursdp)) {
        cGroupDesc *gd = cursdp->groups();
        if (gd) {
            BBox *BBp = &AOI;
            if (!AOI.left && !AOI.bottom && !AOI.right && !AOI.top)
                BBp = 0;
            tlist<sSubcInst> *s0 = 0, *se = 0;
            for (const sSubcList *s = gd->subckts(); s; s = s->next()) {
                for (sSubcInst *sb = s->subs(); sb; sb = sb->next()) {
                    if (!sb->cdesc() || !sb->cdesc()->master())
                        continue;
                    if (name && *name &&
                            strcmp(name, Tstring(sb->cdesc()->cellname())))
                        continue;
                    if (BBp && !BBp->intersect(&sb->cdesc()->oBB(), true))
                        continue;
                    if (!s0)
                        s0 = se = new tlist<sSubcInst>(sb, 0);
                    else {
                        se->next = new tlist<sSubcInst>(sb, 0);
                        se = se->next;
                    }
                }
            }
            sHdl *hdl = new sHdlSubckt(s0, gd);
            res->type = TYP_HANDLE;
            res->content.value = hdl->id;
        }
    }
    return (OK);
}


// (string) GetPscName(subckt_handle)
//
// This function returns the cell name corresponding to the subcircuit
// instance referenced by the handle.  if an error occurs, a null
// string is returned.
//
bool
extract_funcs::IFgetPscName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su && su->elt->cdesc()) {
            res->content.string =
                lstring::copy(Tstring(su->elt->cdesc()->cellname()));
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) GetPscIndex(subckt_handle)
//
// This function returns the index of the subcircuit referenced by the
// argument.  The index is a 0-based sequence for each subcircuit
// master.  If an error occurs, -1 is returned.
//
bool
extract_funcs::IFgetPscIndex(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su)
            res->content.value = su->elt->index();
    }
    return (OK);
}


// (int) GetPscIdNum(subckt_handle)
//
// This function returns the ID number of the subcircuit referenced by
// the argument.  The ID number is unique among all instances in the
// parent cell.  If an error occurs, -1 is returned.
//
bool
extract_funcs::IFgetPscIdNum(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su)
            res->content.value = su->elt->uid();
    }
    return (OK);
}


// (string) GetPscInstName(subckt_handle)
//
// This function returns an instance name corresponding to the
// subcircuit instance referenced by the handle.  This is the cell
// name, followed by an underscore, followed by the index number.  if
// an error occurs, a null string is returned.
//
bool
extract_funcs::IFgetPscInstName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su && su->elt->cdesc()) {
            res->content.string = su->elt->instance_name();
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (object_handle) GetPscDual(subckt_handle)
//
// This function returns an object handle which references the subcell
// in the electrical database which is the dual of the physical
// subcircuit referenced by the argument.  If the association fails, 0
// is returned.
//
bool
extract_funcs::IFgetPscDual(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su && su->elt->dual()) {
            CDol *ol0 = new CDol(su->elt->dual()->cdesc(), 0);
            sHdl *nhdl = new sHdlObject(ol0, cursde);
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (int) GetPscBB(subckt_handle, array)
//
// This function returns the bounding box of the subcircuit referenced
// by the first argument.  The coordinates, in microns relative to the
// origin of the current physical cell, are returned in the array,
// which must have size 4 or larger.  If the operation succeeds, 1 is
// returned, otherwise 0 is returned.
//
bool
extract_funcs::IFgetPscBB(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 4))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su && su->elt->cdesc()) {
            vals[0] = MICRONS(su->elt->cdesc()->oBB().left);
            vals[1] = MICRONS(su->elt->cdesc()->oBB().bottom);
            vals[2] = MICRONS(su->elt->cdesc()->oBB().right);
            vals[3] = MICRONS(su->elt->cdesc()->oBB().top);
            res->content.value = 1;
        }
    }
    return (OK);
}


// (int) GetPscLoc(subckt_handle, array)
//
// This returns the instance placement location, in microns, in the
// array passed as a second argument.  The array must have size two or
// larger.  On success, the function returns 1, and the array location
// 0 will contain the X value, and the 1 location will contain the Y
// value.  Zero is returned on error, with the array values undefined.
//
bool
extract_funcs::IFgetPscLoc(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    double *vals;
    ARG_CHK(arg_array(args, 1, &vals, 2))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su && su->elt->cdesc()) {
            vals[0] = MICRONS(su->elt->cdesc()->posX());
            vals[1] = MICRONS(su->elt->cdesc()->posY());
            res->content.value = 1;
        }
    }
    return (OK);
}


// (string) GetPscTransform(subckt_handle, type, array)
//
// This function returns a string describing the instance orientation. 
// There are presently three format types, specified by the second
// argument.  If this argument is zero, then the Xic transformation
// string is returned.  This is the same CIF-like encoding as used for
// the current transformation in the status line of Xic.  In this case
// the third argument is ignored and can be zero.
//
// If the second argument is one, the return will be a Cadence DEF
// orientation code.  In addition, if an array of size two or larger
// is passed as a third argument, the values will be filled in with
// the X and Y origin correction values implied by the transformation. 
// In a DEF transformation, the lower left corner position of the
// bounding box is invariant, implying that there is an additional
// translation after rotation/mirroring to enforce this.  Pass 0 for
// this argument if these values aren't needed.
//
// In DEF, there is no support for 45, 135, 225, and 315 rotations, a
// null string is returned in these cases.  Magnification is ignored.
//
// If the second argument is any other value, the OpenAccess strings
// are returned, otherwise all is as for DEF.
//
// The following table lists equivalent orientation codes for DEF,
// OpenAccess, and Xic.  The "Origin" column indicates the position of
// the original lower-left corner after the operation.
//
//  LEF/DEF   OpenAccess  Xic      Origin
//  N         R0          R0       LL
//  W         R90         R90      LR
//  S         R180        R180     UR
//  E         R270        R270     UL
//  FN        MY          MX       LR
//  FW        MX90        R270MY   LL
//  FS        MX          MY       UL
//  FE        MY90        R90MX    UR
//
bool
extract_funcs::IFgetPscTransform(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))
    int type;
    ARG_CHK(arg_int(args, 1, &type))
    double *vals;
    ARG_CHK(arg_array_if(args, 2, &vals, 2))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su && su->elt->cdesc()) {
            res->type = TYP_STRING;
            CDtx tx;
            su->elt->cdesc()->get_tx(&tx);
            if (type == 0) {
                // Xic transform string, don't care about array.
                res->content.string = tx.tfstring();
            }
            else {
                // DEF or OA transform string.  DEF assumes that the
                // placement coordinate is the lower-left corner of
                // the placed instance bounding box.  The defstring
                // function computes the correction factor.

                CDc *cd = su->elt->cdesc();
                CDs *msd = cd->masterCell();

                int dx, dy;
                res->content.string = lstring::copy(tx.defstring((type != 1),
                    msd->BB()->width(), msd->BB()->height(), 
                    msd->BB()->left, msd->BB()->bottom, &dx, &dy));
                if (vals) {
                    vals[0] = MICRONS(dx);
                    vals[1] = MICRONS(dy);
                }
            }
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (subc_contact_handle) ListPscContacts(subckt_handle)
//
// This function returns a handle to a list of subcircuit contacts
// associated with the subcircuit referenced by the handle.  The
// returned handle is a distinct type, in particualr subcircuit
// contacts are different from device contacts.  The return handle can
// be used with the functions which query information about subcircuit
// contacts, or with the generic handle functions.  If an error
// occurs, this function returns 0.
//
bool
extract_funcs::IFlistPscContacts(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLsubckt)
            return (BAD);
        tlist<sSubcInst> *su = (tlist<sSubcInst>*)hdl->data;
        if (su) {
            tlist<sSubcContactInst> *sc0 = 0, *sce = 0;
            for (sSubcContactInst *s = su->elt->contacts(); s; s = s->next()) {
                if (!sc0)
                    sc0 = sce = new tlist<sSubcContactInst>(s, 0);
                else {
                    sce->next = new tlist<sSubcContactInst>(s, 0);
                    sce = sce->next;
                }
            }
            sHdl *nhdl = new sHdlSubcContact(sc0, ((sHdlSubckt*)hdl)->gdesc);
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (int) IsPscContactIgnorable(subc_contact_handle)
//
// If the subcircuit associated with the contact referenced from the
// argument is flattened or ignored, return 1.  Otherwise 0 is
// returned.  When 1 is returned, the contact can usually be skipped
// in listings.
//
bool
extract_funcs::IFisPscContactIgnorable(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s)
            res->content.value =
                EX()->shouldFlatten(s->elt->subc()->cdesc(), cursdp);
    }
    return (OK);
}


// (string) GetPscContactName(subc_contact_handle)
//
// This function returns a name string, if available, from the
// subcircuit contact referenced by the argument.  If the subcircuit
// does not provide a name, the returned string will be a number
// giving the subcircuit group contacted.  A null string is returned
// on error.
//
bool
extract_funcs::IFgetPscContactName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s) {
            int grp = s->elt->subc_group();
            CDc *cdesc = s->elt->subc()->cdesc();
            if (cdesc) {
                CDs *msdesc = cdesc->masterCell(true);
                cGroupDesc *gd = msdesc->groups();
                if (gd) {
                    bool ft, cf;
                    const char *nm = gd->group_name(grp, &ft, &cf);
                    if (nm) {
                        res->content.string = lstring::copy(nm);
                        res->flags |= VF_ORIGINAL;
                    }
                }
            }
            if (!res->content.string) {
                char tbuf[32];
                mmItoA(tbuf, grp);
                res->content.string = lstring::copy(tbuf);
                res->flags |= VF_ORIGINAL;
            }
        }
    }
    return (OK);
}


// (int) GetPscContactGroup(subc_contact_handle)
//
// This function returns the group index in the current cell
// corresponding to the subcircuit contact referenced by the argument.
// If an error occurs, this function returns -1.
//
bool
extract_funcs::IFgetPscContactGroup(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s)
            res->content.value = s->elt->parent_group();
    }
    return (OK);
}


// (int) GetPscContactSubcGroup(subc_contact_handle)
//
// This function returns the group index in the subcircuit associated
// with the subcircuit contact referenced by the argument.  On error,
// the function returns -1.
//
bool
extract_funcs::IFgetPscContactSubcGroup(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s)
            res->content.value = s->elt->subc_group();
    }
    return (OK);
}


// (subckt_handle) GetPscContactSubc(subc_contact_handle)
//
// This function returns a handle to the subcircuit which is
// associated with the subcircuit contact referenced by the argument.
// On error, the function return 0.
//
bool
extract_funcs::IFgetPscContactSubc(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s) {
            tlist<sSubcInst> *su = new tlist<sSubcInst>(s->elt->subc(), 0);
            sHdl *nhdl = new sHdlSubckt(su, ((sHdlSubcContact*)hdl)->gdesc);
            res->type = TYP_HANDLE;
            res->content.value = nhdl->id;
        }
    }
    return (OK);
}


// (string) GetPscContactSubcName(subc_contact_handle)
//
// This function returns a string containing the name of the
// subcircuit associatd with the contact referenced by the argument.
// A null string is returned on error.
//
bool
extract_funcs::IFgetPscContactSubcName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s && s->elt->subc() && s->elt->subc()->cdesc()) {
            res->content.string =
                lstring::copy(Tstring(s->elt->subc()->cdesc()->cellname()));
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) GetPscContactSubcIndex(subc_contact_handle)
//
// This function returns the index of the subcircuit associated with
// the contact referenced by the argument.  Each subcircuit of a given
// kind has an index number that is unique in the containing cell.  On
// error, -1 is returned.  Valid index values are 0 and larger.
//
bool
extract_funcs::IFgetPscContactSubcIndex(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s && s->elt->subc())
            res->content.value = s->elt->subc()->index();
    }
    return (OK);
}


// (int) GetPscContactSubcIdNum(subc_contact_handle)
//
// This function returns the ID number of the subcircuit associated
// with the contact referenced by the argument.  Each subcircuit has
// an ID number that is unique in the containing cell.  On error, -1
// is returned.  Valid index values are 0 and larger.

//
bool
extract_funcs::IFgetPscContactSubcIdNum(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s && s->elt->subc())
            res->content.value = s->elt->subc()->uid();
    }
    return (OK);
}


// (string) GetPscContactInstName(subc_contact_handle)
//
// This function returns a string containing an instance name of the
// subcircuit associated with the contact referenced by the argument. 
// The instance name consists of the cellname followed by an
// underscore, which is followed by the index.  A null string is
// returned on error.
//
bool
extract_funcs::IFgetPscContactSubcInstName(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_STRING;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type != HDLscontact)
            return (BAD);
        tlist<sSubcContactInst> *s = (tlist<sSubcContactInst>*)hdl->data;
        if (s && s->elt->subc() && s->elt->subc()->cdesc()) {
            res->content.string = s->elt->subc()->instance_name();
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Electrical Devices
//-------------------------------------------------------------------------

// (stringlist_handle) ListElecDevs(regex)
//
// This function returns a handle to a list of strings containing
// device names from the electrical database.  The names correspond to
// devices used in the current circuit.  The argument is a regular
// expression used to filter the device names.  If the argument is
// null or empty, all devices are listed.  This function returns 0 on
// error.
//
bool
extract_funcs::IFlistElecDevs(Variable *res, Variable *args, void*)
{
    const char *regx;
    ARG_CHK(arg_string(args, 0, &regx))

    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return (BAD);
    regex_t preg;
    if (regx && *regx) {
        if (regcomp(&preg, regx, REG_EXTENDED | REG_NOSUB))
            return (BAD);
    }
    stringlist *s0 = 0;
    CDm_gen mgen(cursde, GEN_MASTERS);
    for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
        CDc_gen cgen(m);
        for (CDc *cd = cgen.c_first(); cd; cd = cgen.c_next()) {
            CDp_name *pn = (CDp_name*)cd->prpty(P_NAME);
            if (!pn)
                continue;
            const char *instname = cd->getBaseName(pn);
            if (!regx || !*regx || !regexec(&preg, instname, 0, 0, 0))
                s0 = new stringlist(lstring::copy(instname), s0);
        }
    }
    if (regx && *regx)
        regfree(&preg);
    sHdl *nhdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = nhdl->id;
    return (OK);
}


// (int) SetEdevProperty(devname, prpty, string)
//
// This function is used to set property values of electrical devices
// and mutual inductors.  It is equivalent to the Set() command, or
// the keyboard !set command, with the @devname.prpty syntax.  The
// first argument is the name of a device in the current circuit.
// This is the value of a Name property for some device.  The second
// argument is a string giving the property type to set or modify.
// The possible strings are "model", "value", "param", "other".
// Actually, only a prefix to these strings is required, so that "m",
// "v", etc.  are accepted.  If the string is unrecognized, the
// property type defaults to "other".  If the device is a mutual
// inductor, only the "name" and "value" properties can be applied.
// The final argument is a string containing the body of the property.
// If the string is null or empty, the property is removed (or reset
// to the default in the case of the "name" property).  The function
// returns 1 on success, 0 otherwise.
//
bool
extract_funcs::IFsetEdevProperty(Variable *res, Variable *args, void*)
{
    const char *dname;
    ARG_CHK(arg_string(args, 0, &dname))
    const char *prpty;
    ARG_CHK(arg_string(args, 1, &prpty))
    const char *string;
    ARG_CHK(arg_string(args, 2, &string))

    if (!dname)
        return (BAD);
    if (!prpty)
        return (BAD);
    res->type = TYP_SCALAR;
    char buf[128];
    sprintf(buf, "%s.%s", dname, prpty);
    res->content.value = SCD()->setDevicePrpty(buf, string);
    return (OK);
}


// (string) GetEdevProperty(devname, prpty)
//
// This function returns a string containing the text of the
// specified property for the given device.  The two arguments have
// the same format and interpretation as the first two arguments of
// SetEdevProperty(), i.e., the device name and property name.  The
// return value is a string containing the text for that property.  If
// the device or property does not exist or some other error occurs, a
// null string is returned.
//
bool
extract_funcs::IFgetEdevProperty(Variable *res, Variable *args, void*)
{
    const char *dname;
    ARG_CHK(arg_string(args, 0, &dname))
    const char *prpty;
    ARG_CHK(arg_string(args, 1, &prpty))

    if (!dname)
        return (BAD);
    if (!prpty)
        return (BAD);
    res->type = TYP_STRING;
    char buf[128];
    sprintf(buf, "%s.%s", dname, prpty);
    res->content.string = SCD()->getDevicePrpty(buf);
    if (res->content.string)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (object_handle) GetEdevObj(devname)
//
// This function returns a handle to the electrical subcell from the
// device library corresponding to the given device name.  If an error
// occurs, 0 is returned.
//
bool
extract_funcs::IFgetEdevObj(Variable *res, Variable *args, void*)
{
    const char *dname;
    ARG_CHK(arg_string(args, 0, &dname))

    if (!dname)
        return (BAD);
    CDs *cursde = CurCell(Electrical, true);
    if (!cursde)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    CDc *cdesc = cursde->findInstance(dname);
    if (cdesc) {
        CDol *ol = new CDol(cdesc, 0);
        sHdl *nhdl = new sHdlObject(ol, cursde);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Resistance Extraction
//-------------------------------------------------------------------------

// (real) ExtractRL(conductor_zoidlist, layername, r_or_l, array, term, ...)
//
// This will use the square-counting system to estimate the resistance
// or inductance of a conducting object with respect to two or more
// terminals.  The first argument is a trapezoid list representing a
// single conducting area, on the layer given in the second argument.
// The layer keywords set electrical parameters used in the
// estimation.
//
// For Resistance:
//   The Resistance layer keyword gives the ohms-per-square of the
//   material.  If not set, a value of 1.0 is assumed.
//
// For Inductance:
//   The Tranline keyword supplies the appropriate parameters.  In
//   this case, the material is assumed to be over a ground plane
//   covered by dielectric.
//
// The third argument is a boolean which if nonzero indicates
// inductance estimation, and zero indicates resistance estimation.
//
// The fourth argument is an array which will hold the return values,
// which will be resized if necessary.  The zeroth component of the
// array gives the number of returned values, which are returned in
// the rest of the array.  If there are two terminals, the number of
// returned values is 1.  For more than two terminals, the number of
// returned values is n*(n-1)/2, where n is the number of terminals.
// The values are the effective two-terminal decomposition for
// terminals i,j (i != j) in the order, e.g., for n = 4, 01, 02, 03,
// 12, 13, 23.
//
// The following arguments are trapezoid lists representing the
// terminals.  Arguments that are not trapezoid lists will be ignored.
// There must be at least two terminals passed.  Terminal areas should
// be spatially disjoint, and in the computation, the terminal areas
// are clipped by the conductor area.  Terminals are assigned numbers
// in left-to-right order.
//
// The algorithm is most efficient if all coordinates are on some
// grid.  This provide for efficient tiling of the structure.
//
// Structures that require a very large number of tiles may require
// excessive time and memory to compute, and/or suffer from a loss of
// accuracy.  The approximate threshold is 1e5 tiling squares.
// Non-Manhattan shapes have strict internal limiting of tile count.
// Manhattan structures can require an arbitrarily large number of
// tiles, thus the potential for resource overuse.
//
// The return value is always 1.  The function will fail (terminating
// the script) if an error is encountered.
//
bool
extract_funcs::IFextractRL(Variable *res, Variable *args, void *datap)
{
    res->type = TYP_SCALAR;
    res->content.value = 0;
    Zlist *zl;
    bool free_zl;
    XIrt xrt = arg_zlist(args, 0, &zl, &free_zl, datap);
    if (xrt != XIok) {
        if (xrt == XIbad)
            return (BAD);
        SI()->SetInterrupt();
        return (OK);
    }
    const char *layername;
    if (!arg_string(args, 1, &layername)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    bool exl;
    if (!arg_boolean(args, 2, &exl)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }
    double *vals;
    if (!arg_array(args, 3, &vals, 1)) {
        FREE_CHK_ZL(free_zl, zl)
        return (BAD);
    }

    CDl *ld = CDldb()->findLayer(layername, Physical);
    if (!ld) {
        FREE_CHK_ZL(free_zl, zl)
        SIparse()->pushError("Unknown layer given");
        return (BAD);
    }

    Zgroup zg;
    zg.list = new Zlist*[MAXARGC];

    int cnt = 0;
    for (;; cnt++) {
        if (args[cnt+4].type == TYP_ENDARG)
            break;
        if (args[cnt+4].type != TYP_ZLIST)
            continue;
        Zlist *zc;
        bool free_zc;
        xrt = arg_zlist(args, cnt+4, &zc, &free_zc, datap);
        if (xrt != XIok) {
            FREE_CHK_ZL(free_zl, zl)
            if (xrt == XIbad)
                return (BAD);
            SI()->SetInterrupt();
            return (OK);
        }
        if (zc) {
            if (!free_zc)
                zg.list[zg.num] = Zlist::copy(zc);
            else
                zg.list[zg.num] = zc;
            zg.num++;
        }
    }

    RLsolver r;
    bool ret = r.setup(zl, ld, &zg);
    FREE_CHK_ZL(free_zl, zl)
    if (!ret) {
        SIparse()->pushError("Solver setup failed: %s",
            Errs()->get_error());
        return (BAD);
    }
    if (exl)
        ret = r.setupL();
    else
        ret = r.setupR();
    if (!ret) {
        SIparse()->pushError("Solver init failed: %s",
            Errs()->get_error());
        return (BAD);
    }
    if (zg.num > 2) {
        int sz;
        float *mat;
        ret = r.solve_multi(&sz, &mat);
        if (!ret) {
            SIparse()->pushError("Solver failed: %s",
                Errs()->get_error());
            return (BAD);
        }

        int nsz = (sz*(sz-1))/2 + 1;
        if (nsz > args[3].content.a->length()) {
            if (ADATA(args[3].content.a)->resize(nsz) == BAD)
                return (BAD);
            vals = args[3].content.a->values();
        }
        vals[0] = sz;
        double *rp = vals + 1;
        for (int i = 0; i < sz; i++) {
            for (int j = i+1; j < sz; j++)
                *rp++ = -1.0/mat[i*sz + j];
        }
        delete [] mat;
    }
    else {
        int nsz = 2;
        if (nsz > args[3].content.a->length()) {
            if (ADATA(args[3].content.a)->resize(nsz) == BAD)
                return (BAD);
            vals = args[3].content.a->values();
        }
        vals[0] = 1;
        ret = r.solve_two(vals+1);
        if (!ret) {
            SIparse()->pushError("Solver failed: %s",
                Errs()->get_error());
            return (BAD);
        }
    }
    res->content.value = ret;
    return (OK);
}


// ExtractNetResistance(net_handle, spicefile, array, term, ...)
//
// This function will extract resistance of a conductor net, taking
// into account multiple conducting layers connected by vias.  The
// resistance decomposition of each conducting object and its vias
// and/or terminals is computed using the algorthm used by the
// ExtractRL function.  The resistance of the connected network is
// then computed, with respect to the terminals specified.
//
// The first argument is a handle to a list of objects as returned
// from FindPath or FindPathOfGroup.
//
// The second argument is a string giving a file name, which will
// contain a generated SPICE listing representing the extracted
// resistor network.  In the SPICE file, each terminal and each via
// are assigned node numbers.  A comment indicates the range of
// numbers used for terminals.  If this argument is 0 (NULL) or an
// empty string, no SPICE file is written.
//
// The third argument is an array which will hold the return values,
// which will be resized if necessary.  The zeroth component of the
// array gives the number of returned values, which are returned in
// the rest of the array.  If there are two terminals, the number of
// returned values is 1.  For more than two terminals, the number of
// returned values is n*(n-1)/2, where n is the number of terminals.
// The values are the effective two-terminal decomposition for
// terminals i,j (i != j) in the order, e.g., for n = 4, 01, 02, 03,
// 12, 13, 23.
//
// The following arguments are trapezoid lists representing the
// terminals.  There must be at least two terminals passed.  Terminal
// areas should be spatially disjoint, and in the computation, the
// terminal areas are clipped by the conductor area.  Terminals are
// assigned numbers in left-to-right order.
//
// The return value is always 1.  The function will fail (terminating
// the script) if an error is encountered.
//
bool
extract_funcs::IFextractNetResistance(Variable *res, Variable *args,
    void *datap)
{
    int net_id;
    ARG_CHK(arg_handle(args, 0, &net_id))
    const char *spicefile;
    ARG_CHK(arg_string(args, 1, &spicefile))
    double *vals;
    ARG_CHK(arg_array_if(args, 2, &vals, 1))

    sHdl *hdl = sHdl::get(net_id);
    if (!hdl || hdl->type != HDLobject) {
        SIparse()->pushError("bad netlist handle");
        return (BAD);
    }
    if (!((sHdlObject*)hdl)->copies) {
        SIparse()->pushError("netlist handle not copies");
        return (BAD);
    }

    Zgroup zg;
    zg.list = new Zlist*[MAXARGC];

    int cnt = 3;
    for (;; cnt++) {
        if (args[cnt].type == TYP_ENDARG)
            break;
        if (args[cnt].type != TYP_ZLIST) {
            SIparse()->pushError("terminal argument not a zlist");
            return (BAD);
        }
        Zlist *zc;
        bool free_zc;
        XIrt xrt = arg_zlist(args, cnt, &zc, &free_zc, datap);
        if (xrt != XIok) {
            if (xrt == XIbad)
                return (BAD);
            SI()->SetInterrupt();
            return (OK);
        }
        if (zc) {
            if (!free_zc)
                zg.list[zg.num] = Zlist::copy(zc);
            else
                zg.list[zg.num] = zc;
            zg.num++;
        }
    }
    if (zg.num < 2) {
        SIparse()->pushError("too few terminals");
        return (BAD);
    }

    CDol *ol = (CDol*)hdl->data;
    if (!ol) {
        SIparse()->pushError("empty netlist handle");
        return (BAD);
    }
    hdl->data = 0;  // The object list is consumed.

    res->type = TYP_SCALAR;
    res->content.value = 0;

    MRsolver r;
    bool ret = r.load_path(ol);
    if (!ret) {
        SIparse()->pushError(Errs()->get_error());
        return (BAD);
    }
    for (int i = 0; i < zg.num; i++) {
        ret = r.add_terminal(zg.list[i]);
        if (!ret) {
            SIparse()->pushError(Errs()->get_error());
            return (BAD);
        }
    }

    ret = r.find_vias();
    if (!ret) {
        SIparse()->pushError(Errs()->get_error());
        return (BAD);
    }

    ret = r.solve_elements();
    if (!ret) {
        SIparse()->pushError(Errs()->get_error());
        return (BAD);
    }

    if (spicefile && *spicefile) {
        FILE *fp = fopen(spicefile, "w");
        if (!fp) {
            Errs()->sys_error("fopen");
            SIparse()->pushError(Errs()->get_error());
            return (BAD);
        }
        ret = r.write_spice(fp);
        fclose(fp);
        if (!ret) {
            SIparse()->pushError(Errs()->get_error());
            return (BAD);
        }
    }

    if (vals) {
        int sz;
        float *mat;
        ret = r.solve(&sz, &mat);
        if (!ret) {
            SIparse()->pushError(Errs()->get_error());
            return (BAD);
        }

        int nsz = (sz == 1 ? 2 : (sz*(sz-1))/2 + 1);
        if (nsz > args[3].content.a->length()) {
            if (ADATA(args[3].content.a)->resize(nsz) == BAD)
                return (BAD);
            vals = args[3].content.a->values();
        }
        vals[0] = sz;
        if (sz == 1)
            vals[1] = mat[0];
        else {
            double *rp = vals + 1;
            for (int i = 0; i < sz; i++) {
                for (int j = i+1; j < sz; j++)
                    *rp++ = -1.0/mat[i*sz + j];
            }
        }
        delete [] mat;
    }
    res->content.value = ret;
    return (OK);
}

