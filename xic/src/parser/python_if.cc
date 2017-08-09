
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
#include "python_if.h"


//-----------------------------------------------------------------------------
// Interface to Python interpreter.
//

namespace {
    const char *notavail = "Python interface not available.";
}

class cPy_nogo : public cPy_base
{
    const char *id_string() { return (notavail); }
    bool init(SymTab*)      { Errs()->add_error(notavail); return (false); }
    bool run(int, char**)   { Errs()->add_error(notavail); return (false); }
    bool runString(const char*, const char*)
                            { Errs()->add_error(notavail); return (false); }
    bool runModuleFunc(const char*, const char*, Variable*, Variable*)
                            { Errs()->add_error(notavail); return (false); }
    void reset()            { }
    PyObject *wrapper(const char*, PyObject*, int, SIscriptFunc)
                            { return (0); }
};


SymTab *cPyIf::py_functions = 0;
cPyIf *cPyIf::instancePtr = 0;

cPyIf::cPyIf(cPy_base *py)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cPyIf already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    pyPtr = py;
    if (pyPtr)
        pyAvail = true;
    else {
        pyPtr = new cPy_nogo;
        pyAvail = false;
    }
}


// Private static error exit.
//
void
cPyIf::on_null_ptr()
{
    fprintf(stderr, "Singleton class cPyIf used before instantiated.\n");
    exit(1);
}


bool
cPyIf::run(const char *prog, const char *s)
{
    bool ret = pyPtr->init(py_functions);
    delete py_functions;
    py_functions = 0;
    if (!ret)
        return (false);

    if (!s)
        s = "";
    stringlist *s0 = 0, *se = 0;
    char *tok;
    while ((tok = lstring::getqtok(&s)) != 0) {
        if (!s0)
            s0 = se = new stringlist(tok, 0);
        else {
            se->next = new stringlist(tok, 0);
            se = se->next;
        }
    }
    int ac = stringlist::length(s0) + 1;
    char **av = new char*[ac+1];
    av[0] = lstring::copy(prog);
    int i = 1;
    while (s0) {
        av[i++] = s0->string;
        s0->string = 0;
        stringlist *sx = s0;
        s0 = s0->next;
        delete sx;
    }
    av[ac] = 0;

    ret = pyPtr->run(ac, av);
    for (i = 0; i < ac; i++)
        delete [] av[i];
    delete [] av;
    return (ret);
}


bool
cPyIf::runString(const char *prmstr, const char *cmds)
{
    bool ret = pyPtr->init(py_functions);
    delete py_functions;
    py_functions = 0;
    if (!ret)
        return (false);
    return (pyPtr->runString(prmstr, cmds));
}


bool
cPyIf::runModuleFunc(const char *mname, const char *fname, Variable *res,
    Variable *args)
{
    Errs()->init_error();
    if (!mname || !*mname) {
        Errs()->add_error("runModuleFunc: null or empty module name.");
        return (false);
    }
    if (!fname || !*fname) {
        Errs()->add_error("runModuleFunc: null or empty function name.");
        return (false);
    }

    if (py_functions) {
        bool ret = pyPtr->init(py_functions);
        delete py_functions;
        py_functions = 0;
        if (!ret)
            return (false);
    }
    return (pyPtr->runModuleFunc(mname, fname, res, args));
}


void
cPyIf::reset()
{
    pyPtr->reset();
}


// Static function.
// This puts a Python function implementation into the table.  The table
// must be filled before Python is run, at which time the table will be
// loaded into the Python interpreter and destroyed.
//
// The fname MUST be a persistent string!
//
void
cPyIf::register_func(const char *fname, PyObject*(*func)(PyObject*, PyObject*))
{
    if (!fname || !func)
        return;
    if (!py_functions)
        py_functions = new SymTab(false, false);
    SymTabEnt *ent = SymTab::get_ent(py_functions, fname);
    if (!ent)
        py_functions->add(fname, (void*)func, false);
    else
        ent->stData = (void*)func;
}

