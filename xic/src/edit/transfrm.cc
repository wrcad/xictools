
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
#include "dsp_tkif.h"
#include "dsp_inlines.h"


//
// Current Transform manipulations.
//

// Set the current transform.
//
void
cEdit::setCurTransform(int rot, bool mx, bool my, double magn)
{
    if (magn <= 0.0)
        magn = 1.0;
    else if (magn < 0.001)
        magn = 0.001;
    else if (magn > 1000.0)
        magn = 1000.0;
    sCurTx ct;
    ct.set_reflectY(my);
    ct.set_reflectX(mx);
    int da = (DSP()->CurMode() == Physical ? 45 : 90);
    ct.set_angle(((rot + da/2)/da)*da);
    if (DSP()->CurMode() == Physical)
        ct.set_magn(magn);
    DSPmainDraw(ShowGhost(ERASE))
    GEO()->setCurTx(ct);
    DSPmainDraw(ShowGhost(DISPLAY))
    PopUpTransform(0, MODE_UPD, 0, 0);
    XM()->ShowParameters();
}


// Set the current transform via the string.
//
bool
cEdit::setCurTransform(const char *str)
{
    sCurTx ct;
    bool ret = ct.parse_tform_string(str, (DSP()->CurMode() == Electrical));
    if (ret) {
        DSPmainDraw(ShowGhost(ERASE))
        GEO()->setCurTx(ct);
        DSPmainDraw(ShowGhost(DISPLAY))
        PopUpTransform(0, MODE_UPD, 0, 0);
        XM()->ShowParameters();
    }
    return (ret);
}


// Function to cycle through the rotation angle choices, called from
// key press handler.
//
void
cEdit::incrementRotation(bool inc)
{
    sCurTx ct = *GEO()->curTx();
    // The set_angle method keeps saved value in range.
    if (DSP()->CurMode() == Physical) {
        if (inc)
            ct.set_angle(GEO()->curTx()->angle() + 45);
        else
            ct.set_angle(GEO()->curTx()->angle() - 45);
    }
    else {
        if (inc)
            ct.set_angle(GEO()->curTx()->angle() + 90);
        else
            ct.set_angle(GEO()->curTx()->angle() - 90);
    }
    DSPmainDraw(ShowGhost(ERASE))
    GEO()->setCurTx(ct);
    DSPmainDraw(ShowGhost(DISPLAY))
    PopUpTransform(0, MODE_UPD, 0, 0);
    XM()->ShowParameters();
}


// Alter the current transform to map Y to -Y.
//
void
cEdit::flipY()
{
    sCurTx ct = *GEO()->curTx();
    ct.set_reflectY(!GEO()->curTx()->reflectY());
    DSPmainDraw(ShowGhost(ERASE))
    GEO()->setCurTx(ct);
    DSPmainDraw(ShowGhost(DISPLAY))
    PopUpTransform(0, MODE_UPD, 0, 0);
    XM()->ShowParameters();
}


// Alter the current transform to map X to -X.
//
void
cEdit::flipX()
{
    sCurTx ct = *GEO()->curTx();
    ct.set_reflectX(!GEO()->curTx()->reflectX());
    DSPmainDraw(ShowGhost(ERASE))
    GEO()->setCurTx(ct);
    DSPmainDraw(ShowGhost(DISPLAY))
    PopUpTransform(0, MODE_UPD, 0, 0);
    XM()->ShowParameters();
}


#define NREG 6

namespace {
    sCurTx PtfmBak[NREG];
    sCurTx EtfmBak[NREG];
}


// Store current transform parameters.
//
void
cEdit::saveCurTransform(int reg)
{
    if (reg < 0 || reg >= NREG)
        reg = 0;
    sCurTx &tb =
        (DSP()->CurMode() == Physical ? PtfmBak[reg] : EtfmBak[reg]);
    tb = *GEO()->curTx();
}


// Recall current transform parameters.
//
void
cEdit::recallCurTransform(int reg)
{
    if (reg < 0 || reg >= NREG)
        reg = 0;
    sCurTx &tb =
        (DSP()->CurMode() == Physical ? PtfmBak[reg] : EtfmBak[reg]);
    DSPmainDraw(ShowGhost(ERASE))
    GEO()->setCurTx(tb);
    DSPmainDraw(ShowGhost(DISPLAY))
    PopUpTransform(0, MODE_UPD, 0, 0);
    XM()->ShowParameters();
}


// Clear the current transform.
//
void
cEdit::clearCurTransform()
{
    sCurTx ct;
    DSPmainDraw(ShowGhost(ERASE))
    GEO()->setCurTx(ct);
    DSPmainDraw(ShowGhost(DISPLAY))
    PopUpTransform(0, MODE_UPD, 0, 0);
    XM()->ShowParameters();
}


// Static function.
// Callback for the XFORM command in physical and electrical side menu.
//
bool
cEdit::cur_tf_cb(const char *name, bool state, const char *string, void*)
{
    if (name) {
        if (!strcmp(name, "rflx")) {
            sCurTx ct = *GEO()->curTx();
            ct.set_reflectX(state);
            GEO()->setCurTx(ct);
            XM()->ShowParameters();
            return (true);
        }
        if (!strcmp(name, "rfly")) {
            sCurTx ct = *GEO()->curTx();
            ct.set_reflectY(state);
            GEO()->setCurTx(ct);
            XM()->ShowParameters();
            return (true);
        }
        if (!strcmp(name, "ang")) {
            sCurTx ct = *GEO()->curTx();
            ct.set_angle(atoi(string));
            GEO()->setCurTx(ct);
            XM()->ShowParameters();
            return (true);
        }
        if (!strcmp(name, "magn")) {
            char *endp;
            double d = strtod(string, &endp);
            if (endp > string && d >= CDMAGMIN && d <= CDMAGMAX) {
                sCurTx ct = *GEO()->curTx();
                ct.set_magn(d);
                GEO()->setCurTx(ct);
                XM()->ShowParameters();
            }
            return (true);
        }
    }
    return (true);
}

