
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

#ifndef TCLTK_IF_H
#define TCLTK_IF_H

#include "tcl_base.h"


typedef void *ClientData;
struct Variable;

inline class cTclIf *TclIf();


// Main class to interface Xic to Tcl/Tk.
//
class cTclIf
{
    static cTclIf *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cTclIf *TclIf() { return (cTclIf::ptr()); }

    cTclIf(cTcl_base*);
    virtual ~cTclIf() { }

    // Capability flags.
    bool hasTcl()               { return (tclAvail); }
    bool hasTk()                { return (tkAvail); }

    // Wrapper for native script functions.  This is never called unless
    // the tclPtr is good.
    int wrapper(const char *n, Tcl_Interp *interp, Tcl_Obj *const *args,
            int acnt, int nargs, SIscriptFunc xicfunc)
        {
            return (tclPtr->wrapper(n, interp, args, acnt, nargs, xicfunc));
        }

    bool run(const char*);
    bool runString(const char*, const char*);
    void reset();

    static void register_func(const char*, int(*)(ClientData, Tcl_Interp*,
        int, Tcl_Obj* const*));

private:
    cTcl_base *tclPtr;
    bool tclAvail;
    bool tkAvail;

    static SymTab *tcl_functions;
    static cTclIf *instancePtr;
};


// Macro to define a Tcl function.
#define TCL_FUNC(name, na, f) \
  int tcl##name(ClientData, Tcl_Interp *interp, int ac, Tcl_Obj *const *a) \
  { return (TclIf()->wrapper(#name, interp, a, ac, na, &f)); }

#endif

