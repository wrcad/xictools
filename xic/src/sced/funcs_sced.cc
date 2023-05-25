
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
#include "sced.h"
#include "sced_nodemap.h"
#include "edit.h"
#include "extif.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_assemble.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "si_parsenode.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "si_interp.h"


namespace {
    namespace sced_funcs {

        // Output Generation
        bool IFconnect(Variable*, Variable*, void*);
        bool IFtoSpice(Variable*, Variable*, void*);

        // Electrical Nodes
        bool IFincludeNoPhys(Variable*, Variable*, void*);
        bool IFgetNumberNodes(Variable*, Variable*, void*);
        bool IFsetNodeName(Variable*, Variable*, void*);
        bool IFgetNodeName(Variable*, Variable*, void*);
        bool IFgetNodeNumber(Variable*, Variable*, void*);
        bool IFgetNodeGroup(Variable*, Variable*, void*);
        bool IFlistNodePins(Variable*, Variable*, void*);
        bool IFlistNodeContacts(Variable*, Variable*, void*);
        bool IFgetNodeContactInstance(Variable*, Variable*, void*);
        bool IFlistNodePinNames(Variable*, Variable*, void*);
        bool IFlistNodeContactNames(Variable*, Variable*, void*);

        // Symbolic Mode
        bool IFisShowSymbolic(Variable*, Variable*, void*);
        bool IFshowSymbolic(Variable*, Variable*, void*);
        bool IFsetSymbolicFast(Variable*, Variable*, void*);
        bool IFmakeSymbolic(Variable*, Variable*, void*);
    }
    using namespace sced_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    // Conversion = Export
    PY_FUNC(Connect,                1,  IFconnect);
    PY_FUNC(ToSpice,                1,  IFtoSpice);

    // Electrical Nodes
    PY_FUNC(IncludeNoPhys,          1,  IFincludeNoPhys);
    PY_FUNC(GetNumberNodes,         0,  IFgetNumberNodes);
    PY_FUNC(SetNodeName,            2,  IFsetNodeName);
    PY_FUNC(GetNodeName,            1,  IFgetNodeName);
    PY_FUNC(GetNodeNumber,          1,  IFgetNodeNumber);
    PY_FUNC(GetNodeGroup,           1,  IFgetNodeGroup);
    PY_FUNC(ListNodePins,           1,  IFlistNodePins);
    PY_FUNC(ListNodeContacts,       1,  IFlistNodeContacts);
    PY_FUNC(GetNodeContactInstance, 1,  IFgetNodeContactInstance);
    PY_FUNC(ListNodePinNames,       1,  IFlistNodePinNames);
    PY_FUNC(ListNodeContactNames,   1,  IFlistNodeContactNames);

    // Symbolic Mode
    PY_FUNC(IsShowSymbolic,         0,  IFisShowSymbolic);
    PY_FUNC(ShowSymbolic,           1,  IFshowSymbolic);
    PY_FUNC(SetSymbolicFast,        1,  IFsetSymbolicFast);
    PY_FUNC(MakeSymbolic,           0,  IFmakeSymbolic);


    void py_register_sced()
    {
      // Conversion = Export
      cPyIf::register_func("Connect",                pyConnect);
      cPyIf::register_func("ToSpice",                pyToSpice);

      // Electrical Nodes
      cPyIf::register_func("IncludeNoPhys",          pyIncludeNoPhys);
      cPyIf::register_func("GetNumberNodes",         pyGetNumberNodes);
      cPyIf::register_func("SetNodeName",            pySetNodeName);
      cPyIf::register_func("GetNodeName",            pyGetNodeName);
      cPyIf::register_func("GetNodeNumber",          pyGetNodeNumber);
      cPyIf::register_func("GetNodeGroup",           pyGetNodeGroup);
      cPyIf::register_func("ListNodePins",           pyListNodePins);
      cPyIf::register_func("ListNodeContacts",       pyListNodeContacts);
      cPyIf::register_func("GetNodeContactInstance", pyGetNodeContactInstance);
      cPyIf::register_func("ListNodePinNames",       pyListNodePinNames);
      cPyIf::register_func("ListNodeContactNames",   pyListNodeContactNames);

      // Symbolic Mode
      cPyIf::register_func("IsShowSymbolic",         pyIsShowSymbolic);
      cPyIf::register_func("ShowSymbolic",           pyShowSymbolic);
      cPyIf::register_func("SetSymbolicFast",        pySetSymbolicFast);
      cPyIf::register_func("MakeSymbolic",           pyMakeSymbolic);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // TclTk wrappers.

    // Conversion = Export
    TCL_FUNC(Connect,                1,  IFconnect);
    TCL_FUNC(ToSpice,                1,  IFtoSpice);

    // Electrical Nodes
    TCL_FUNC(IncludeNoPhys,          1,  IFincludeNoPhys);
    TCL_FUNC(GetNumberNodes,         0,  IFgetNumberNodes);
    TCL_FUNC(SetNodeName,            2,  IFsetNodeName);
    TCL_FUNC(GetNodeName,            1,  IFgetNodeName);
    TCL_FUNC(GetNodeNumber,          1,  IFgetNodeNumber);
    TCL_FUNC(GetNodeGroup,           1,  IFgetNodeGroup);
    TCL_FUNC(ListNodePins,           1,  IFlistNodePins);
    TCL_FUNC(ListNodeContacts,       1,  IFlistNodeContacts);
    TCL_FUNC(GetNodeContactInstance, 1,  IFgetNodeContactInstance);
    TCL_FUNC(ListNodePinNames,       1,  IFlistNodePinNames);
    TCL_FUNC(ListNodeContactNames,   1,  IFlistNodeContactNames);

    // Symbolic Mode
    TCL_FUNC(IsShowSymbolic,         0,  IFisShowSymbolic);
    TCL_FUNC(ShowSymbolic,           1,  IFshowSymbolic);
    TCL_FUNC(SetSymbolicFast,        1,  IFsetSymbolicFast);
    TCL_FUNC(MakeSymbolic,           0,  IFmakeSymbolic);


    void tcl_register_sced()
    {
      // Conversion = Export
      cTclIf::register_func("Connect",                tclConnect);
      cTclIf::register_func("ToSpice",                tclToSpice);

      // Electrical Nodes
      cTclIf::register_func("IncludeNoPhys",          tclIncludeNoPhys);
      cTclIf::register_func("GetNumberNodes",         tclGetNumberNodes);
      cTclIf::register_func("SetNodeName",            tclSetNodeName);
      cTclIf::register_func("GetNodeName",            tclGetNodeName);
      cTclIf::register_func("GetNodeNumber",          tclGetNodeNumber);
      cTclIf::register_func("GetNodeGroup",           tclGetNodeGroup);
      cTclIf::register_func("ListNodePins",           tclListNodePins);
      cTclIf::register_func("ListNodeContacts",       tclListNodeContacts);
      cTclIf::register_func("GetNodeContactInstance", tclGetNodeContactInstance);
      cTclIf::register_func("ListNodePinNames",       tclListNodePinNames);
      cTclIf::register_func("ListNodeContactNames",   tclListNodeContactNames);

      // Symbolic Mode
      cTclIf::register_func("IsShowSymbolic",         tclIsShowSymbolic);
      cTclIf::register_func("ShowSymbolic",           tclShowSymbolic);
      cTclIf::register_func("SetSymbolicFast",        tclSetSymbolicFast);
      cTclIf::register_func("MakeSymbolic",           tclMakeSymbolic);
    }
#endif  // HAVE_TCL
}


// Export to load functions in this script library.
//
void
cSced::loadScriptFuncs()
{
  using namespace sced_funcs;

  // Conversion = Export
  SIparse()->registerFunc("Connect",                1,  IFconnect);
  SIparse()->registerFunc("ToSpice",                1,  IFtoSpice);

  // Electrical Nodes
  SIparse()->registerFunc("IncludeNoPhys",          1,  IFincludeNoPhys);
  SIparse()->registerFunc("GetNumberNodes",         0,  IFgetNumberNodes);
  SIparse()->registerFunc("SetNodeName",            2,  IFsetNodeName);
  SIparse()->registerFunc("GetNodeName",            1,  IFgetNodeName);
  SIparse()->registerFunc("GetNodeNumber",          1,  IFgetNodeNumber);
  SIparse()->registerFunc("GetNodeGroup",           1,  IFgetNodeGroup);
  SIparse()->registerFunc("ListNodePins",           1,  IFlistNodePins);
  SIparse()->registerFunc("ListNodeContacts",       1,  IFlistNodeContacts);
  SIparse()->registerFunc("GetNodeContactInstance", 1,  IFgetNodeContactInstance);
  SIparse()->registerFunc("ListNodePinNames",       1,  IFlistNodePinNames);
  SIparse()->registerFunc("ListNodeContactNames",   1,  IFlistNodeContactNames);

  // Symbolic Mode
  SIparse()->registerFunc("IsShowSymbolic",         0,  IFisShowSymbolic);
  SIparse()->registerFunc("ShowSymbolic",           1,  IFshowSymbolic);
  SIparse()->registerFunc("SetSymbolicFast",        1,  IFsetSymbolicFast);
  SIparse()->registerFunc("MakeSymbolic",           0,  IFmakeSymbolic);

#ifdef HAVE_PYTHON
  py_register_sced();
#endif
#ifdef HAVE_TCL
  tcl_register_sced();
#endif
}


//-------------------------------------------------------------------------
// Output Generation
//-------------------------------------------------------------------------

// (int) Connect(for_spice)
//
// This function establishes the circuit connectivity for the current
// hierarchy.  If the boolean for_spice is false, then devices with
// the nophys property set are ignored, and the netlist will have the
// "shorted" nophys devices shorted out.  This is appropriate for LVS
// and other extraction system operations.
//
// If for_spice is true, the NOPHYS devices are included, and not
// shorted.  This applies when generating output for SPICE simulation.
//
// The function returns 1 on success, 0 otherwise.  If the schematic
// is already processed and current, the function will return
// immediately.  The schematic is implicitly processed before most
// internal operations that make use of the schematic, so it is
// unlikely that the user will need to call this function.
//
bool
sced_funcs::IFconnect(Variable *res, Variable *args, void*)
{
    bool forsp;
    ARG_CHK(arg_boolean(args, 0, &forsp))

    // implicit "Commit"
    ED()->ulCommitChanges();

    res->type = TYP_SCALAR;
    res->content.value = SCD()->connectAll(forsp);
    return (OK);
}


// (int) ToSpice(spicefile)
//
// This command will dump a SPICE file from the current cell to a file
// of the given name.  If the argument is null or an empty string, the
// name will be that of the current cell with a .cir suffix.  Any
// existing file of the same name will be moved, and given a .bak
// extension.  The return value is 1 on success, 0 otherwise.
//
bool
sced_funcs::IFtoSpice(Variable *res, Variable *args, void*)
{
    const char *spicefile;
    ARG_CHK(arg_string(args, 0, &spicefile))

    if (!DSP()->CurCellName())
        return (BAD);
    char buf[256];
    if (!spicefile || !*spicefile) {
        strcpy(buf, Tstring(DSP()->CurCellName()));
        strcat(buf, ".cir");
        spicefile = buf;
    }

    // implicit "Commit"
    ED()->ulCommitChanges();

    res->type = TYP_SCALAR;
    res->content.value = SCD()->dumpSpiceFile(spicefile);
    return (OK);
}


//-------------------------------------------------------------------------
// Electrical Nodes
//-------------------------------------------------------------------------

// (boolean) IncludeNoPhys(flag)
//
// This sets an internal mode which applies to the other functions in
// this group.  If the boolean flag argument is nonzero, devices with
// the NOPHYS property set will be considered when generating the
// connectivity and node mapping structures.  This has relevance when
// a device has the shorted option to NOPHYS set, as such devices will
// be considered as normal devices with the flag set.  If the flag is
// unset, these devices will be taken as short circuits, which of
// course alters the node assignments.
//
// Internally, the extraction functions always take these devices as
// shorted, and they are otherwise ignored.  When generating a SPICE
// file during simulation or with other commands in the side menu,
// these devices are included as normal devices.  The present state of
// the netlist data structures will reflect the state of the last
// operation.
//
// Setting this flag will cause rebuilding of the data structures to
// the requested state if necessary when one of the functions in this
// section is called.  This persists until some other function, such
// as an extraction or SPICE listing function is called, at which time
// the internal state of the flag may change.  Thus, this function may
// need to be called repeatedly ahead of the functions in this
// section.
//
// The return value is the previous value of the internal flag.
//
bool
sced_funcs::IFincludeNoPhys(Variable *res, Variable *args, void*)
{
    bool on;
    ARG_CHK(arg_boolean(args, 0, &on))

    res->type = TYP_SCALAR;
    res->content.value = SCD()->includeNoPhys();
    SCD()->setIncludeNoPhys(on);
    return (OK);
}


// (int) GetNumberNodes()
//
// Return the size of the internal node map.  The internal node
// numbers range from 0 up to but not including this value.  The
// return value is 0 on error or if the cell is empty.
//
bool
sced_funcs::IFgetNumberNodes(Variable *res, Variable*, void*)
{
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    SCD()->connect(cursde);
    cNodeMap *map = cursde->nodes();
    if (map)
        res->content.value = map->countNodes();
    return (OK);
}


// (int) SetNodeName(node, name)
//
// This function associates the string name with the node number given
// in the first argument.  This affects the electrical database, and
// is equivalent to setting a node name with the node mapping facility
// available in the side menu in electrical mode.  Netlist output will
// use the given string name rather than a default name, however if
// the existing default name matches a global node name, the
// user-supplied name will be ignored.  If the name given is null or
// empty, any existing given name is deleted, and netlist output will
// use the node number.  The function returns 1 on success, 0
// otherwise.
//
bool
sced_funcs::IFsetNodeName(Variable *res, Variable *args, void*)
{
    int node;
    ARG_CHK(arg_int(args, 0, &node))
    const char *name;
    ARG_CHK(arg_string(args, 1, &name))

    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    SCD()->connect(cursde);
    cNodeMap *map = cursde->nodes();
    if (map) {
        int size = map->countNodes();
        if (node > 0 && node < size) {
            if (name && *name) {
                int nx = map->findNode(name);
                if (nx >= 0) {
                    // Existing name already in use.
                    if (nx == node)
                        // New name == old name, no error.
                        res->content.value = 1;
                    return (OK);
                }
                res->content.value = map->newEntry(name, node);
            }
            else {
                map->delEntry(node);
                res->content.value = 1;
            }
        }
    }
    return (OK);
}


// (string) GetNodeName(node)
//
// This function returns a string name for the given node number.  If
// a name has been given for that node, the name is returned,
// otherwise an internally generated default name is returned.  If the
// operation fails, a null string is returned.
//
bool
sced_funcs::IFgetNodeName(Variable *res, Variable *args, void*)
{
    int node;
    ARG_CHK(arg_int(args, 0, &node))

    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = 0;
    SCD()->connect(cursde);
    cNodeMap *map = cursde->nodes();
    if (map) {
        if (node >= 0 && node < map->countNodes()) {
            const char *s = map->mapName(node);
            char buf[64];
            if (!*s) {
                snprintf(buf, sizeof(buf), "%d", node);
                s = buf;
            }
            res->content.string = lstring::copy(s);
            res->flags |= VF_ORIGINAL;
        }
    }
    return (OK);
}


// (int) GetNodeNumber(name)
//
// This function returns the node number corresponding to the name
// string passed as an argument.  If no mapping to the string is
// found, -1 is returned.
//
bool
sced_funcs::IFgetNodeNumber(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    if (!name)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    SCD()->connect(cursde);
    res->content.value = SCD()->findNode(cursde, name);
    return (OK);
}


// (int) GetNodeGroup(node)
//
// This function returns the group index in the physical cell that
// corresponds to the given node number.  On error, -1 is returned.
//
bool
sced_funcs::IFgetNodeGroup(Variable *res, Variable *args, void*)
{
    int node;
    ARG_CHK(arg_int(args, 0, &node))

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = -1;
    if (ExtIf()->associate(cursdp)) {
        res->content.value =
            ExtIf()->groupOfNode(cursdp, node);
    }
    return (OK);
}


// (terminal_handle) ListNodePins(node)
//
// Note:  This and ListNodeContacts replace ListNodeTerminals, which
// was removed in 4.2.12.
//
// Return a handle to the list of cell connection terminals bound to the
// internal node number supplied as the argument.  There probably will
// be at most one such connection.
//
bool
sced_funcs::IFlistNodePins(Variable *res, Variable *args, void*)
{
    int node;
    ARG_CHK(arg_int(args, 0, &node))

    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    SCD()->connect(cursde);

    tlist2<CDp_nodeEx> *t0 = 0, *te = 0;
    for (CDp_snode *ps = (CDp_snode*)cursde->prpty(P_NODE); ps;
            ps = ps->next()) {
        if (ps->enode() == node) {
            if (!t0)
                t0 = te = new tlist2<CDp_nodeEx>(ps, cursde, 0);
            else {
                te->next = new tlist2<CDp_nodeEx>(ps, cursde, 0);
                te = te->next;
            }
        }
    }
    if (t0) {
        sHdl *nhdl = new sHdlNode(t0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    return (OK);
}


// (terminal_handle) ListNodeContacts(node)
//
// Note:  This and ListNodePins replace ListNodeTerminals, which was
// removed in 4.2.12.
//
// Return a handle to a list of device and subcircuit connection
// terminals bound to the specified node.
//
bool
sced_funcs::IFlistNodeContacts(Variable *res, Variable *args, void*)
{
    int node;
    ARG_CHK(arg_int(args, 0, &node))

    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    SCD()->connect(cursde);

    tlist2<CDp_nodeEx> *t0 = 0, *te = 0;
    CDg gdesc;
    gdesc.init_gen(cursde, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (!cdesc->is_normal())
            continue;
        CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pc; pc = pc->next()) {
            if (pc->enode() == node) {
                if (!t0)
                    t0 = te = new tlist2<CDp_nodeEx>(pc, cdesc, 0);
                else {
                    te->next = new tlist2<CDp_nodeEx>(pc, cdesc, 0);
                    te = te->next;
                }
            }
        }
    }
    if (t0) {
        sHdl *nhdl = new sHdlNode(t0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    return (OK);
}


// (object_handle) GetNodeContactInstance(terminal_handle)
//
// For a handle to an instance contact, such as returned from
// ListNodeContacts, this function will return a handle to the device
// or subcircuit instance that provides the contact.
//
bool
sced_funcs::IFgetNodeContactInstance(Variable *res, Variable *args, void*)
{
    int id;
    ARG_CHK(arg_handle(args, 0, &id))

    sHdl *hdl = sHdl::get(id);
    res->type = TYP_SCALAR;
    res->content.string = 0;
    if (hdl) {
        if (hdl->type == HDLnode) {
            tlist2<CDp_nodeEx> *t = (tlist2<CDp_nodeEx>*)hdl->data;
            if (t && t->elt) {
                CDp_cnode *pc = dynamic_cast<CDp_cnode*>(t->elt);
                if (pc) {
                    CDc *cd = (CDc*)t->xtra;
                    if (!cd) {
                        // Shouldn't get here,
                        CDcterm *cterm = pc->inst_terminal();
                        if (cterm)
                            cd = cterm->instance();
                    }
                    if (cd) {
                        CDol *ol = new CDol(cd, 0);
                        hdl = new sHdlObject(ol, cd->parent());
                        res->type = TYP_HANDLE;
                        res->content.value = hdl->id;
                    }
                }
            }
        }
        else if (hdl->type == HDLterminal) {
            tlist<CDterm> *t = (tlist<CDterm>*)hdl->data;
            if (t && t->elt) {
                CDcterm *cterm = dynamic_cast<CDcterm*>(t->elt);
                if (cterm) {
                    CDc *inst = cterm->instance();
                    if (inst) {
                        CDol *ol = new CDol(inst, 0);
                        hdl = new sHdlObject(ol, inst->parent());
                        res->type = TYP_HANDLE;
                        res->content.value = hdl->id;
                    }
                }
            }
        }
        else
            return (BAD);
    }
    return (OK);
}


// (strnglist_handle) ListNodePinNames(node)
//
// Note:  This and ListNodeContactNames replace
// ListNodeTerminalNames, which was removed in 4.2.12.
//
// Return a list of cell connection terminal names that connect to the
// given node.  There is likely at most one cell connection per node.
//
bool
sced_funcs::IFlistNodePinNames(Variable *res, Variable *args, void*)
{
    int node;
    ARG_CHK(arg_int(args, 0, &node))

    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    SCD()->connect(cursde);

    stringlist *s0 = 0, *se = 0;
    for (CDp_snode *ps = (CDp_snode*)cursde->prpty(P_NODE); ps;
            ps = ps->next()) {
        if (ps->enode() == node) {
            if (!s0) {
                s0 = se = new stringlist(
                    lstring::copy(Tstring(ps->term_name())), 0);
            }
            else {
                se->next = new stringlist(
                    lstring::copy(Tstring(ps->term_name())), 0);
                se = se->next;
            }
        }
    }
    if (s0) {
        sHdl *nhdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    return (OK);
}


// (strnglist_handle) ListNodeContactNames(node)
//
// Note:  This and ListNodePinNames replace
// ListNodeTerminalNames, which was removed in 4.2.12.
//
// Return a list of device and subcircuit contact names that connect
// to the given node.
//
bool
sced_funcs::IFlistNodeContactNames(Variable *res, Variable *args, void*)
{
    int node;
    ARG_CHK(arg_int(args, 0, &node))

    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 0;
    SCD()->connect(cursde);

    stringlist *s0 = 0, *se = 0;
    CDg gdesc;
    gdesc.init_gen(cursde, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (!cdesc->is_normal())
            continue;
        CDp_cnode *pc = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pc; pc = pc->next()) {
            if (pc->enode() == node) {
                if (!s0) {
                    s0 = se = new stringlist(
                        lstring::copy(Tstring(pc->term_name())), 0);
                }
                else {
                    se->next = new stringlist(
                        lstring::copy(Tstring(pc->term_name())), 0);
                    se = se->next;
                }
            }
        }
    }
    if (s0) {
        sHdl *nhdl = new sHdlString(s0);
        res->type = TYP_HANDLE;
        res->content.value = nhdl->id;
    }
    return (OK);
}


//-------------------------------------------------------------------------
// Symbolic Mode
//-------------------------------------------------------------------------

// IsShowSymbolic()
//
// This function will return 1 if the current cell is being displayed
// in symbolic form in the main window, 0 otherwise.  The return is
// always 0 in physical mode.
//
bool
sced_funcs::IFisShowSymbolic(Variable *res, Variable*, void*)
{
    res->type = TYP_SCALAR;
    res->content.value = CurCell() && CurCell()->isElectrical() &&
        CurCell()->isSymbolic();
    return (OK);
}


// ShowSymbolic(show)
//
// This will set symbolic mode of the current cell, and display the
// symbolic representation, if possible, in the main window.  The
// effect is similar to the effect of pressing the symbl button in the
// electrical side menu.  The function call must be made in electrical
// display mode.  When symbolic mode is asserted, by passing a boolean
// true argument, the current cell will be displayed in symbolic mode,
// unless the No Top Symbolic button in the Main Window sub-menu of
// the Attributes menu is pressed.  The return value is 1 on success,
// 0 if some error occurred, with an error message likely available
// from GetError.
//
bool
sced_funcs::IFshowSymbolic(Variable *res, Variable *args, void*)
{
    bool show;
    ARG_CHK(arg_boolean(args, 0, &show))

    res->type = TYP_SCALAR;
    res->content.value = SCD()->setCurSymbolic(show);
    return (OK);
}


// (int) SetSymbolicFast(symb)
//
// This will enable or disable symbolic mode of the current cell.  It
// differs from ShowSymbolic in two ways.  First, it applies only to
// cells with a symbolic representation, meaning that it has a
// symbolic form which may or may not be visible.  Second, it will
// change the status of a flag in the cell, but there will be no
// updating of the screen or other internal things (such as undo
// logging).  The caller must reset to the original state before a
// screen redisplay or any major operation.  This is much faster than
// calling ShowSymbolic, and can be used when making quick changes to
// a cell.
//
// The return value is 1 if the current cell was previously actively
// symbolic, 0 otherwise.  The return value is always 0, and the
// function has no effect, in physical mode.
//
bool
sced_funcs::IFsetSymbolicFast(Variable *res, Variable *args, void*)
{
    bool symb;
    ARG_CHK(arg_boolean(args, 0, &symb))

    res->type = TYP_SCALAR;
    res->content.value = SCD()->setCurSymbolicFast(symb);
    return (OK);
}


// MakeSymbolic()
//
// This will create a very simple symbolic representation of the
// electrical view of the current cell, consisting of a box with a
// name label, and wire stubs containing the terminals.  Any existing
// symbolic representation will be overwritten (but the operation can
// be undone).  In electrical mode, symbolic mode will be asserted.
//
// On success, 1 is returned, 0 otherwise.
//
bool
sced_funcs::IFmakeSymbolic(Variable *res, Variable*, void*)
{
    // implicit "Commit"
    ED()->ulCommitChanges();

    res->type = TYP_SCALAR;
    res->content.value = SCD()->makeSymbolic();
    return (OK);
}

