
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: python_if.h,v 5.11 2015/07/04 22:19:50 stevew Exp $
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

    cPyIf();
    virtual ~cPyIf() { }

    // Capability flag.
    bool hasPy()            { return (has_py); }

    // Wrapper for native script functions.  This is never called unless
    // the pyPtr is good.
    PyObject *wrapper(const char *n, PyObject *args, int nargs,
            SIscriptFunc xicfunc)
        {
            return (pyPtr->wrapper(n, args, nargs, xicfunc));
        }

    bool run(const char*);
    bool runString(const char*, const char*);
    bool runModuleFunc(const char*, const char*, Variable*, Variable*);
    void reset();

    static void register_func(const char*, PyObject*(*)(PyObject*, PyObject*));

private:
    static void bang_py(const char*);

    cPy_base *pyPtr;
    bool has_py;

    static SymTab *py_functions;
    static cPyIf *instancePtr;
};

// Macro to define a Python function.
#define PY_FUNC(name, na, f) PyObject *py##name(PyObject*, PyObject *a) \
  { return (PyIf()->wrapper(#name, a, na, &f)); }

#endif

