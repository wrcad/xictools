
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

#include "cd.h"
#include "tcltk_if.h"
#include "tcl_base.h"


//-----------------------------------------------------------------------------
// Interface to Tcl/Tk interpreter.
//

namespace {
    const char *notavail = "Tcl/Tk interface not available.";
}

class cTcl_nogo : public cTcl_base
{
    const char *id_string() { return (notavail); }
    bool init(SymTab*)      { Errs()->add_error(notavail); return (false); }
    bool run(const char*, bool)
                            { Errs()->add_error(notavail); return (false); }
    bool runString(const char*, const char*)
                            { Errs()->add_error(notavail); return (false); }
    void reset()            { }
    int wrapper(const char*, Tcl_Interp*, Tcl_Obj* const*, int, int,
                            SIscriptFunc)
                            { return (0); }
    bool hasTk()            { return (false); }
};


SymTab *cTclIf::tcl_functions = 0;
cTclIf *cTclIf::instancePtr = 0;

cTclIf::cTclIf(cTcl_base *tcl)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cTclIf already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    tclPtr = tcl;
    if (tclPtr) {
        tclAvail = true;
        tkAvail = tclPtr->hasTk();
    }
    else {
        tclPtr = new cTcl_nogo;
        tclAvail = false;
        tkAvail = false;
    }
}


// Private static error exit.
//
void
cTclIf::on_null_ptr()
{
    fprintf(stderr, "Singleton class cTclIf used before instantiated.\n");
    exit(1);
}


bool
cTclIf::run(const char *s)
{
    Errs()->init_error();
    const char *cmdstr = s;
    char *cmdfile = lstring::getqtok(&s);
    if (!cmdfile) {
        Errs()->add_error("run: no command file given!");
        return (false);
    }
    GCarray<char*> gc_cmdfile(cmdfile);
    bool is_tk = false;
    if (tkAvail) {
        // Tk is available.  The file must have a .tcl or .tk extension.
        char *ext = strrchr(cmdfile, '.');
        if (!ext || (!lstring::cieq(ext, ".tcl") &&
                !lstring::cieq(ext, ".tk"))) {
            Errs()->add_error(
                "run: command file must have .tcl or .tk extension.");
            return (false);
        }
        is_tk = lstring::cieq(ext, ".tk");

    }
    else {
        // We allow any extension but .tk.
        char *ext = strrchr(cmdfile, '.');
        if (ext && lstring::cieq(ext, ".tk")) {
            Errs()->add_error("run: .tk extension, Tk is unavailable.");
            return (false);
        }
    }
    if (tcl_functions) {
        bool ret = tclPtr->init(tcl_functions);
        tcl_functions = 0;
        if (!ret)
            return (false);
    }
    return (tclPtr->run(cmdstr, is_tk));
}


bool
cTclIf::runString(const char *prmstr, const char *cmds)
{
    Errs()->init_error();
    if (tcl_functions) {
        bool ret = tclPtr->init(tcl_functions);
        tcl_functions = 0;
        if (!ret)
            return (false);
    }
    return (tclPtr->runString(prmstr, cmds));
}


void
cTclIf::reset()
{
    tclPtr->reset();
}


// Static function.
// This puts a Tcl/Tk function implementation into the table.  The table
// must be filled before Tcl/Tk is run, at which time the table will be
// loaded into the Tcl/Tk interpreter and destroyed.
//
// The fname MUST be a persistent string!
//
void
cTclIf::register_func(const char *fname,
    int(*func)(ClientData, Tcl_Interp*, int, Tcl_Obj* const*))
{
    if (!fname || !func)
        return;
    if (!tcl_functions)
        tcl_functions = new SymTab(false, false);
    SymTabEnt *ent = SymTab::get_ent(tcl_functions, fname);
    if (!ent)
        tcl_functions->add(fname, (void*)func, false);
    else
        ent->stData = (void*)func;
}

