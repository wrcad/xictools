
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
 $Id: py_base.h,v 5.9 2015/07/04 22:19:50 stevew Exp $
 *========================================================================*/

#ifndef PY_BASE_H
#define PY_BASE_H

#include "si_scrfunc.h"


// Base class for Python interpreter plug-in.
//
class cPy_base
{
public:
    virtual ~cPy_base() { }
    virtual const char *id_string() = 0;
    virtual bool init(SymTab*) = 0;
    virtual bool run(int, char**) = 0;
    virtual bool runString(const char*, const char*) = 0;
    virtual bool runModuleFunc(const char*, const char*, Variable*,
        Variable*) = 0;
    virtual void reset() = 0;
    virtual PyObject *wrapper(const char*, PyObject*, int,
            SIscriptFunc) = 0;
};

#endif

