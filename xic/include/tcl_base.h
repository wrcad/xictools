
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
 $Id: tcl_base.h,v 5.6 2015/07/04 22:19:51 stevew Exp $
 *========================================================================*/

#ifndef TCL_BASE_H
#define TCL_BASE_H

#include "si_scrfunc.h"


struct Tcl_Interp;
struct Tcl_Obj;

// Base class for Tcl/Tk interpreter plug-in.
//
class cTcl_base
{
public:
    virtual ~cTcl_base() { }
    virtual const char *id_string() = 0;
    virtual bool init(SymTab*) = 0;
    virtual bool run(const char*, bool) = 0;
    virtual bool runString(const char*, const char*) = 0;
    virtual void reset() = 0;
    virtual int wrapper(const char*, Tcl_Interp*, Tcl_Obj* const*, int, int,
        SIscriptFunc) = 0;
    virtual bool hasTk() = 0;
};

#endif

