
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
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
 $Id: si_scrfunc.h,v 1.5 2017/05/01 17:14:03 stevew Exp $
 *========================================================================*/

#ifndef SI_SCRFUNC_H
#define SI_SCRFUNC_H


//
// Export some script function things, mostly for plug-ins.
//

// Maximum number of arguments to functions.
#define MAXARGC 50
#define VARARGS -1

struct Variable;

// Typedef for script evaluation functions.
typedef bool(*SIscriptFunc)(Variable*, Variable*, void*);

// Exports for the plug-in kit.
//
void registerScriptFunc(SIscriptFunc, const char*, int);
void unRegisterScriptFunc(const char*);

#endif

