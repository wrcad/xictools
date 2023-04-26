
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
#include "ext.h"
#include "ext_fc.h"
#include "ext_fxunits.h"
#include "ext_fxjob.h"
#include "dsp_inlines.h"
#include "qtmain.h"
#include "menu.h"  
#include "select.h"


// Pop up a panel to control the fastcap/fasthenry interface.
//
void
cFC::PopUpExtIf(GRobject caller, ShowMode mode)
{
(void)caller;
(void)mode;
/*
    if (!GRX || !QTmainwin::self())
        return;
    if (mode == MODE_OFF) {
        delete Fch;
        return;
    }
    if (mode == MODE_UPD) {
        if (Fch)
            Fch->update();
        return;
    }
    if (Fch)
        return;

    new sFch(caller);
    if (!Fch->Shell()) {
        delete Fch;
        return;
    }

    gtk_window_set_transient_for(GTK_WINDOW(Fch->Shell()),
        GTK_WINDOW(mainBag()->Shell()));
    GRX->SetPopupLocation(GRloc(LW_LR), Fch->Shell(), mainBag()->viewport);
    gtk_widget_show(Fch->Shell());
    setPopUpVisible(true);
*/
}


void
cFC::updateString()
{
/*
    if (Fch) {
        char *s = statusString();
        gtk_label_set(GTK_LABEL(Fch->label), s);
        delete [] s;
    }
*/
}


void
cFC::updateMarks()
{
}


void
cFC::clearMarks()
{
}


/*
void
cFC::freezeParams(bool freeze, fx_params *params)
{
    if (Fch) {
        if (freeze) {
            Fch->frozen = true;
            Fch->set_sens(false);
            double val = MICRONS(params->fx_min_rect_size);
            gtk_adjustment_set_value(GTK_ADJUSTMENT(Fch->fx_min_rect), val);
            val = MICRONS(params->fx_plane_bloat);
            gtk_adjustment_set_value(GTK_ADJUSTMENT(Fch->fx_plane_bloat), val);
            val = MICRONS(params->fc_part_max);
            gtk_adjustment_set_value(GTK_ADJUSTMENT(Fch->fc_part_max), val);
            val = MICRONS(params->fc_edge_max);
            gtk_adjustment_set_value(GTK_ADJUSTMENT(Fch->fc_edge_max), val);
            val = MICRONS(params->fc_thin_edge);
            gtk_adjustment_set_value(GTK_ADJUSTMENT(Fch->fc_thin_edge), val);
        }
        else {
            Fch->frozen = false;
            Fch->set_sens(true);
            if (params)
                params->set();
            App->PopUpExtIf(0, MODE_UPD);
        }
    }
(void)freeze;
(void)params;
}
*/

