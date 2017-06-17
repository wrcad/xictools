
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
 $Id: array.cc,v 1.7 2010/07/22 04:14:49 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "edit.h"
#include "array.h"
#include "undolist.h"


//
// Instance array manipulation functions.
//

// Convert the instance array to individual instances.
//
bool
sArrayManip::unarray()
{
    if (!am_sdesc || !am_cdesc) {
        Errs()->add_error("sArrayManip::unarray: class not initialized.");
        return (false);
    }
    TPush();
    TApplyTransform(am_cdesc);
    CDap ap(am_cdesc);
    int tx, ty;
    CallDesc calldesc;
    am_cdesc->call(&calldesc);
    TGetTrans(&tx, &ty);
    CDap tap;
    for (unsigned int i = 0; i < ap.ny; i++) {
        for (unsigned int j = 0; j < ap.nx; j++) {
            TTransMult(j*ap.dx, i*ap.dy);
            CDtx ttx;
            TCurrent(&ttx);

            CDc *newc;
            if (OIfailed(am_sdesc->makeCall(&calldesc, &ttx, &tap, CDcallNone,
                    &newc))) {
                TPop();
                Errs()->add_error("sArrayManip::unarray: makeCall failed.");
                return (false);
            }

            TSetTrans(tx, ty);
            Ulist()->RecordObjectChange(am_sdesc, 0, newc);
        }
    }
    Ulist()->RecordObjectChange(am_sdesc, am_cdesc, 0);
    am_cdesc = 0;
    TPop();
    return (true);
}


// Reconfigure the array so that the given rectangular index region
// (inclusive) is clear.  The array is replaced by a collection of new
// arrays or instances.
//
bool
sArrayManip::delete_elements(unsigned int nx1, unsigned int nx2,
    unsigned int ny1, unsigned int ny2)
{
    if (!am_sdesc || !am_cdesc) {
        Errs()->add_error(
            "sArrayManip::delete_elements: class not initialized.");
        return (false);
    }

    if (nx2 < nx1)
        SwapInts(nx1, nx2);
    if (ny2 < ny1)
        SwapInts(ny1, ny2);

    CDap ap(am_cdesc);
    if (nx1 >= ap.nx)
        return (true);
    if (ny1 >= ap.ny)
        return (true);
    nx2++;
    ny2++;
    if (nx2 > ap.nx)
        nx2 = ap.nx;
    if (ny2 > ap.ny)
        ny2 = ap.ny;

    BBox BBa(0, 0, ap.nx, ap.ny);
    BBox BBc(nx1, ny1, nx2, ny2);
    Blist *bl0 = BBa.clip_out(&BBc, 0);

    TPush();
    TApplyTransform(am_cdesc);
    bool ret = true;
    for (Blist *bl = bl0; bl; bl = bl->next) {
        if (!mk_instance(&bl->BB)) {
            ret = false;
            break;
        }
    }
    TPop();
    bl0->free();

    if (ret) {
        Ulist()->RecordObjectChange(am_sdesc, am_cdesc, 0);
        am_cdesc = 0;
    }
    return (true);
}


// Reconfigure the array parameters of the instance or array.  This
// can convert instances to arrays and vice-versa.
//
bool
sArrayManip::reconfigure(unsigned int nx, int dx, unsigned int ny, int dy)
{
    if (nx < 1 || ny < 1) {
        Errs()->add_error(
            "sArrayManip::reconfigure: nx or ny less than 1.");
        return (false);
    }
    if (!am_sdesc || !am_cdesc) {
        Errs()->add_error(
            "sArrayManip::reconfigure: class not initialized.");
        return (false);
    }

    CallDesc calldesc;
    am_cdesc->call(&calldesc);
    TPush();
    TApplyTransform(am_cdesc);
    CDtx tx;
    TCurrent(&tx);
    TPop();
    CDap ap(nx, ny, dx, dy);
    CDc *newodesc;
    if (OIfailed(am_sdesc->makeCall(&calldesc, &tx, &ap, CDcallNone,
            &newodesc))) {
        Errs()->add_error("sArrayManip::reconfigure: makeCall failed.");
        return (false);
    }
    newodesc->prptyAddCopyList(am_cdesc->prpty_list());
    Ulist()->RecordObjectChange(am_sdesc, am_cdesc, newodesc);
    am_cdesc = 0;
    return (true);
}


bool
sArrayManip::mk_instance(const BBox *BB)
{
    if (BB->top <= BB->bottom)
        return (true);
    if (BB->right <= BB->left)
        return (true);

    unsigned int ox = BB->left;
    unsigned int nx = BB->width();
    unsigned int oy = BB->bottom;
    unsigned int ny = BB->height();
    CDap ap(am_cdesc);

    int tx, ty;
    TGetTrans(&tx, &ty);
    CDap tap(nx, ny, ap.dx, ap.dy);
    TTransMult(ox*ap.dx, oy*ap.dy);
    CDtx ttx;
    TCurrent(&ttx);

    CallDesc calldesc;
    am_cdesc->call(&calldesc);
    CDc *newc;
    if (OIfailed(am_sdesc->makeCall(&calldesc, &ttx, &tap, CDcallNone,
            &newc))) {
        Errs()->add_error("sArrayManip::mk_instance: makeCall failed.");
        return (false);
    }
    TSetTrans(tx, ty);

    Ulist()->RecordObjectChange(am_sdesc, 0, newc);
    return (true);
}

