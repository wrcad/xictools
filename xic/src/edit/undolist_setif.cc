
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
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
 $Id: undolist_setif.cc,v 1.6 2008/07/13 06:18:06 stevew Exp $
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

