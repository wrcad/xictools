
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
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_lexpr.h"
#include "si_handle.h"
#include "si_interp.h"
#include "crypt.h"


////////////////////////////////////////////////////////////////////////
//
// Script Execution - Support
//
////////////////////////////////////////////////////////////////////////

// Register an application context struct.
//
void
SIinterp::RegisterLocalContext(SIlocal_context *cx)
{
    siLocalContext = cx;
}


// Call this after a script exits for any reason, to perform cleanup.
//
void
SIinterp::Cleanup()
{
    SIparse()->clearExprs();

    sHdl::clear();
    if (siLocalContext)
        siLocalContext->clear();
}


// Update the objects in the handle lists.
//
void
SIinterp::UpdateObject(CDo *oldobj, CDo *newobj)
{
    sHdl::update(oldobj, newobj);
}


// Update the properties in the handle lists.
//
void
SIinterp::UpdatePrpty(CDo *od, CDp *oldp, CDp *newp)
{
    sHdl::update(od, oldp, newp);
}


// Update the cells in the handle lists.
//
void
SIinterp::UpdateCell(CDs *sd)
{
    if (sd == 0)
        sHdl::update();
    else
        sHdl::update(sd);
}


// Update when the group desc is about to be freed.
//
void
SIinterp::ClearGroups(cGroupDesc *g)
{
    sHdl::update(g);
}
// End of SIinterp functions.


// Factory - return a SIfile* ready for use, or 0 if error and set error
// flag.  If no FILE pointer is passed, attempt to open it here.
//
SIfile *
SIfile::create(const char *fn, FILE *f, sif_err *err)
{
    if (err)
        *err = sif_ok;
    if (!f) {
        f = fopen(fn, "rb");
        if (!f) {
            if (err)
                *err = sif_open;
            return (0);
        }
    }
    sCrypt *c = 0;
    if (sCrypt::is_encrypted(f)) {
        char k[13];
        if (!SI()->GetKey(k)) {
            fclose(f);
            if (err)
                *err = sif_crypt;
            return (0);
        }
        c = new sCrypt;
        c->savekey(k);
        c->initialize();
        const char *es;
        if (!c->begin_decryption(f, &es)) {
            delete c;
            fclose(f);
            if (err)
                *err = sif_crypt;
            return (0);
        }
    }
    return (new SIfile(f, c, lstring::copy(fn)));
}


SIfile::~SIfile()
{
    if (fp)
        fclose(fp);
    delete cr;
    delete [] filename;
}


// Get a character, translating if necessary.
//
int
SIfile::sif_getc()
{
    if (!fp)
        return (EOF);
    int c = getc(fp);
    if (c == EOF)
        return (EOF);
    if (cr) {
        unsigned char cb = c;
        cr->translate(&cb, 1);
        c = cb;
    }
    return (c);
}
// End of SIfile functions

