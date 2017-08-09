
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

#ifndef PYTHON_IF_H
#define PYTHON_IF_H

struct PyObject;
#include "py_base.h"


inline class cPyIf *PyIf();

// Main class to interface Xic to Python.
//
class cPyIf
{
    static cPyIf *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cPyIf *PyIf() { return (cPyIf::ptr()); }

    cPyIf(cPy_base*);
    virtual ~cPyIf() { }

    // Capability flag.
    bool hasPy()            { return (pyAvail); }

    // Wrapper for native script functions.  This is never called unless
    // the pyPtr is good.
    PyObject *wrapper(const char *n, PyObject *args, int nargs,
            SIscriptFunc xicfunc)
        {
            return (pyPtr->wrapper(n, args, nargs, xicfunc));
        }

    bool run(const char*, const char*);
    bool runString(const char*, const char*);
    bool runModuleFunc(const char*, const char*, Variable*, Variable*);
    void reset();

    static void register_func(const char*, PyObject*(*)(PyObject*, PyObject*));

private:
    cPy_base *pyPtr;
    bool pyAvail;

    static SymTab *py_functions;
    static cPyIf *instancePtr;
};

// Macro to define a Python function.
#define PY_FUNC(name, na, f) PyObject *py##name(PyObject*, PyObject *a) \
  { return (PyIf()->wrapper(#name, a, na, &f)); }

#endif

