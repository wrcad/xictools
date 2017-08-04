
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

#include "dsp.h"
#include "cd_layer.h"
#include "cd_variable.h"
#include "fio.h"
#include "fio_gdsii.h"
#include "tech.h"
#include "tech_kwords.h"
#include "cvrt_variables.h"


// Parse the per-layer data conversion keywords.
//
TCret
cTech::ParseCvtLayerBlock()
{
    if (Matching(Tkw.StreamData())) {
        // Set the stream layer number and data type for the layer
        // being defined, applies to physical and electrical modes.
        // For backward compatibility.
        //
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        int lnum = 0;
        int dtyp = -1;
        if (sscanf(tc_inbuf, "%d %d", &lnum, &dtyp) < 1)
            return (SaveError("%s: bad or missing data.", Tkw.StreamData()));
        if (lnum < 0 || lnum >= GDS_MAX_LAYERS) {
            return (SaveError("%s: layer number out of range (0..%d).",
                Tkw.StreamData(), GDS_MAX_LAYERS-1));
        }
        if (dtyp < -1 || dtyp >= GDS_MAX_DTYPES) {
            return (SaveError("%s: datatype number out of range (0..%d).",
                Tkw.StreamData(), GDS_MAX_DTYPES-1));
        }
        char buf[32];
        if (dtyp >= 0)
            sprintf(buf, "%d, %d", lnum, dtyp);
        else
            sprintf(buf, "%d", lnum);
        if (!tc_last_layer->setStrmIn(buf)) {
            return (SaveError(
                "%s: failed to set input mapping, unknown error.",
                Tkw.StreamData()));
        }
        if (dtyp < 0)
            dtyp = 0;
        tc_last_layer->addStrmOut(lnum, dtyp);
        return (TCmatch);
    }
    if (Matching(Tkw.StreamIn())) {
        // Set the Stream input mapping
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        if (!tc_last_layer->setStrmIn(tc_inbuf))
            return (SaveError("%s: bad specification.", Tkw.StreamIn()));
        return (TCmatch);
    }
    if (Matching(Tkw.StreamOut())) {
        // Set Stream output mapping
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        int lnum = 0;;
        int dtyp = 0;
        if (sscanf(tc_inbuf, "%d %d", &lnum, &dtyp) < 1)
            return (SaveError("%s: bad or missing data.", Tkw.StreamOut()));
        if (lnum < 0 || lnum >= GDS_MAX_LAYERS) {
            return (SaveError("%s: layer number out of range (0..%d).",
                Tkw.StreamOut(), GDS_MAX_LAYERS-1));
        }
        if (dtyp < 0 || dtyp >= GDS_MAX_DTYPES) {
            return (SaveError("%s: datatype number out of range (0..%d).",
                Tkw.StreamOut(), GDS_MAX_DTYPES-1));
        }
        tc_last_layer->addStrmOut(lnum, dtyp);
        return (TCmatch);
    }
    if (Matching(Tkw.NoDrcDataType())) {
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        int i;
        if (sscanf(tc_inbuf, "%d", &i) == 1 && i >= 0 && i < GDS_MAX_DTYPES) {
            tc_last_layer->setNoDRC(true);
            tc_last_layer->setDatatype(CDNODRC_DT, i);
        }
        else {
            return (SaveError("%s: datatype out of range (0..%d).",
                Tkw.NoDrcDataType(), GDS_MAX_DTYPES-1));
        }
        return (TCmatch);
    }
    return (TCnone);
}


// Print the per-layer conversion keyword lines to fp or lstr.  If
// cmts, also print associated comments.
//
void
cTech::PrintCvtLayerBlock(FILE *fp, sLstr *lstr, bool cmts, const CDl *ld,
    DisplayMode mode)
{
    if (!ld)
        return;
    tBlkType tbt = mode == Physical ? tBlkPlyr : tBlkElyr;
    char buf[256];

    // StreamIn
    if (ld->strmIn()) {
        PutStr(fp, lstr, Tkw.StreamIn());
        PutChr(fp, lstr, ' ');
        ld->strmIn()->print(fp, lstr);
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.StreamIn());

    // StreamOut
    for (strm_odata *so = ld->strmOut(); so; so = so->next()) {
        sprintf(buf, "%s %d %d\n", Tkw.StreamOut(), so->layer(), so->dtype());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tbt, ld->name(), Tkw.StreamOut());

    // NoDrcDataType
    if (mode == Physical) {
        if (ld->isNoDRC()) {
            sprintf(buf, "%s %d\n", Tkw.NoDrcDataType(),
                ld->datatype(CDNODRC_DT));
            PutStr(fp, lstr, buf);
        }
        if (cmts)
            CommentDump(fp, lstr, tbt, ld->name(), Tkw.NoDrcDataType());
    }
}

