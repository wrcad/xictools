
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
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
 $Id: oa_lib_observer.cc,v 1.13 2015/03/26 04:32:16 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "errorlog.h"
#include "oa.h"
#include "oa_lib_observer.h"
#include "oa_errlog.h"


// Constructor which passes the priority argument to its inherited
// constructor.
//
cOAlibObserver::cOAlibObserver(oaUInt4 priorityIn, oaBoolean enabledIn) :
    oaObserver<oaLibDefList>(priorityIn, enabledIn)
{
}


// This overloaded member function of the cOAlibObserver class,
// prints out either a syntax error or any warning which are issued by
// the oaLibDefList object management.
//
oaBoolean
cOAlibObserver::onLoadWarnings(oaLibDefList *list, const oaString &msg,
    oaLibDefListWarningTypeEnum type)
{
    if (type == oacNoDefaultLibDefListWarning) {
        // No warning if there is no lib.defs file found
        return (true);
    }

    oaString libDefPath;
    if (list)
        list->getPath(libDefPath);

    Errs()->add_error(
        "While loading the library definitions file\n %s,\n"
        "OpenAccess generated the message:\n %s",
        (const char*)libDefPath,
        (const char*)msg);
    return (false);
}
// End of cOAlibObserver functions.


void
cOApcellObserver::onError(oaDesign *design, const oaString &msg,
    oaPcellErrorTypeEnum)
{
    oaString libname, cellname, viewname;
    design->getLibName(oaNativeNS(), libname);
    design->getCellName(oaNativeNS(), cellname);
    design->getViewName(oaNativeNS(), viewname);
    sLstr lstr;
    lstr.add("Failed to create sub-master of ");
    lstr.add(libname);
    lstr.add_c('/');
    lstr.add(cellname);
    lstr.add_c('/');
    lstr.add(viewname);
    lstr.add_c('.');
    lstr.add_c('\n');
    lstr.add(msg);
    if (OAerrLog.err_file())
        fprintf(OAerrLog.err_file(), "OpenAccess: %s\n", lstr.string());
    else
        Log()->ErrorLog(mh::OpenAccess, lstr.string());
}


//
// The pcell observer is not used at present.
//

void
cOApcellObserver::onPreEval(oaDesign*, oaPcellDef*)
{
}


void
cOApcellObserver::onPostEval(oaDesign*, oaPcellDef*)
{
}

