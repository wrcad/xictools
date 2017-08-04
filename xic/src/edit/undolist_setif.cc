
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

#include "main.h"
#include "edit.h"
#include "undolist.h"


//-----------------------------------------------------------------------------
// CD configuration

namespace {
    // If create is true, record the hypertext entry in the history list.
    // If create is false, Remove all references to ent in the history
    // list.  Unlike objects and properties, hypertext entries are owned
    // by CD and should not be freed by the application.
    //
    void
    ifRecordHYent(hyEnt *ent, bool create)
    {
        Ulist()->RecordHYent(ent, create);
    }

    // An object (oldesc) is being deleted, and being replaced by newdesc.
    // Either olddesc or newdesc can be null.
    //
    // This is called when the changes are to be recorded in a history
    // (undo) list.  It is up to the application to actually delete old
    // objects.
    //
    void
    ifRecordObjectChange(CDs *sdesc, CDo *olddesc, CDo *newdesc)
    {
        Ulist()->RecordObjectChange(sdesc, olddesc, newdesc);
    }

    // A property (oldp) is being deleted, and being replaced by newp.
    // Either oldp or newp can be null.  If odesc is not null, the
    // properties apply to that object, otherwise they apply to sdesc.
    //
    // This is called when the changes are to be recorded in a history
    // (undo) list.  It is up to the application to actually delete old
    // properties.
    //
    void
    ifRecordPrptyChange(CDs *sdesc, CDo *odesc, CDp *oldp, CDp *newp)
    {
        Ulist()->RecordPrptyChange(sdesc, odesc, oldp, newp);
    }
}


//-----------------------------------------------------------------------------
// GEO configuration

// use ifRecordObjectChange from above


void
cUndoList::setupInterface()
{
    CD()->RegisterIfRecordHYent(ifRecordHYent);
    CD()->RegisterIfRecordObjectChange(ifRecordObjectChange);
    CD()->RegisterIfRecordPrptyChange(ifRecordPrptyChange);

    GEO()->RegisterIfRecordObjectChange(ifRecordObjectChange);
}

