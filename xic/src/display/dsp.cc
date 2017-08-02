
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "config.h"
#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "cd_lgen.h"


//
// Initialization functions for the display system
//

cDisplay *cDisplay::instancePtr = 0;

cDisplay::cDisplay()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cDisplay is already instantiated.\n");
        exit (1);
    }
    instancePtr = this;

    for (int i = 0; i < DSP_NUMWINS; i++)
        d_windows[i] = 0;

    d_incomplete_object     = 0;
    d_rulers = 0;

    d_threads               = 0;
    d_cell_threshold        = 0;
    d_grid_threshold        = 0;
    d_max_label_len         = 0;
    d_max_label_lines       = 0;
    d_sleep_time_ms         = 0;
    d_select_pixel          = 0;
    d_phys_prop_size        = 0;
    d_term_text_size        = 0;
    d_term_mark_size        = 0;

    d_phys_char_width       = 0.0;
    d_phys_char_height      = 0.0;
    d_elec_char_width       = 0.0;
    d_elec_char_height      = 0.0;
    d_mw_pan_factor         = DEF_MW_PAN_FCT;
    d_mw_zoom_factor        = DEF_MW_ZOOM_FCT;

    d_pixel_delta           = DEF_PixelDelta;

    d_interrupt             = DSPinterNone;

    d_erase_behind_terms    = ErbhNone;

    d_in_edge_snap_cmd      = false;
    d_no_grid_snap          = false;
    d_no_coarse_only        = false;

#ifdef GRP_CMAP_ENABLED
    d_grp_cmap              = 0;
    d_use_grp_cmap          = false;
#endif

    d_show_cnums            = false;
    d_show_invis_marks      = false;

    d_no_graphics           = false;
    d_no_redisplay          = false;
    d_slow_mode             = false;
    d_doing_hcopy           = false;
    d_no_pixmap_store       = false;
    d_no_local_image        = false;
    d_no_display_cache      = false;

    d_use_driver_labels     = false;
    d_number_vertices       = false;
    d_show_terminals        = false;
    d_terminals_visible     = false;
    d_contacts_visible      = false;
    d_erase_behind_props    = false;
    d_show_instance_mark    = false;
    d_show_centroid_mark    = false;

    d_hidden_label_mode     = HLall;
    d_cfv_bloat             = 0;
    d_showing_node          = -1;
    d_internal_pixel        = 0;
    d_alloc_count           = 0;
    d_context_dark_pcnt     = DSP_DEF_CX_DARK_PCNT;
    d_fence_inst_pixsz      = DSP_MIN_FENCE_INST_PIXELS;

    d_color_table           = 0;
    d_invisible_master_tab  = 0;
    d_min_cell_width        = 0;
    d_empty_cell_width      = 0;
    d_redisplay_queued      = false;
    d_transform_overflow    = false;
    d_initialized           = false;

    setupInterface();
    createColorTable();
}


// Private static error exit.
//
void
cDisplay::on_null_ptr()
{
    fprintf(stderr, "Singleton class cDisplay used before instantiated.\n");
    exit(1);
}


// Perform some initialization.  The window should have been created
// and set to windows[wnum] before calling this function.  This works
// for the main and sub windows.
//
void
cDisplay::Initialize(int width, int height, int wnum, bool nogr)
{
    if (wnum < 0 || wnum >= DSP_NUMWINS)
        return;
    if (wnum == 0)
        d_no_graphics = nogr;

    WindowDesc *wdesc = d_windows[wnum];
    if (wdesc) {
        wdesc->SetID();
        wdesc->SetDisplFlags(CDexpand << wnum);
        wdesc->SetRatio(1.0);
        wdesc->InitViewport(width, height);
        if (wnum > 0) {
            wdesc->CenterFullView(wdesc->Window());
            wdesc->Wdraw()->SetBackground(Color(BackgroundColor));
            wdesc->Wdraw()->SetWindowBackground(Color(BackgroundColor));
            wdesc->Wdraw()->SetGhostColor(Color(GhostColor));
            wdesc->Wdraw()->Clear();
            return;
        }
    }

    if (!d_initialized) {
        d_initialized           = true;

        d_box_linestyle.mask    = DEF_BoxLineStyle;
        d_phys_char_width       = CDphysDefTextWidth;
        d_phys_char_height      = CDphysDefTextHeight;
        d_elec_char_width       = CDelecDefTextWidth;
        d_elec_char_height      = CDelecDefTextHeight;
        d_max_label_len         = DSP_DEF_MAX_LABEL_LEN;
        d_max_label_lines       = DSP_DEF_MAX_LABEL_LINES;
        d_grid_threshold        = DSP_DEF_GRID_THRESHOLD;
        d_cell_threshold        = DSP_DEF_CELL_THRESHOLD;
        d_sleep_time_ms         = DSP_SLEEP_TIME_MS;
        d_phys_prop_size        = DSP_DEF_PTRM_TXTHT;
        d_term_text_size        = DSP_DEF_PTRM_TXTHT;
        d_term_mark_size        = DSP_DEF_PTRM_DELTA;
        d_show_terminals        = false;

        // Make sure that existing layers have the user_data allocated.
        CDlgen pgen(Physical, CDlgen::BotToTopWithCells);
        CDl *ld;
        while ((ld = pgen.next()) != 0) {
            if (!ld->dspData())
                ld->setDspData(new DspLayerParams(ld));
        }

        CDlgen egen(Electrical, CDlgen::BotToTopWithCells);
        while ((ld = egen.next()) != 0) {
            if (!ld->dspData())
                ld->setDspData(new DspLayerParams(ld));
        }

        ld = CellLayer();
        if (!ld) {
            // Allocate physical cell layer.
            // Should never get here, the cell layer is created in the
            // layer database initialization.

            ld = CDldb()->addNewLayer(CELL_LAYER_NAME, Physical,
                CDLcellInstance, 0);
        }
        ld->setFilled(true);
        dsp_prm(ld)->setColor(0, 0, 0);

        if (!CDldb()->layer(0, Electrical)) {
            // Allocate electrical cell layer.
            CDldb()->insertLayer(ld, Electrical, 0);
        }

        // Do some main window initialization.
        if (wnum == 0 && wdesc->Wdraw()) {
            wdesc->Wdraw()->SetGhostColor(Color(GhostColor));
            wdesc->Wdraw()->defineLinestyle(BoxLinestyle(),
                BoxLinestyle()->mask);
            DSPattrib *a = DSP()->MainWdesc()->Attrib();
            if (a) {
                wdesc->Wdraw()->defineLinestyle(
                    &a->grid(Physical)->linestyle(),
                    a->grid(Physical)->linestyle().mask);
                wdesc->Wdraw()->defineLinestyle(
                    &a->grid(Electrical)->linestyle(),
                    a->grid(Electrical)->linestyle().mask);
            }
        }
    }
}


// Open a new subwindow.  The new window index is returned, or -1 if
// error.
//
int
cDisplay::OpenSubwin(const BBox *AOI, WDdbType type, const char *sdb_name,
    bool sdb_free)
{
    int i;
    for (i = 1; i < DSP_NUMWINS; i++) {
        if (!d_windows[i])
            break;
    }
    WindowDesc *wdesc;
    if (i < DSP_NUMWINS) {
        wdesc = new WindowDesc();
        d_windows[i] = wdesc;
        if (MainWdesc())
            *wdesc->Attrib() = *MainWdesc()->Attrib();
        *wdesc->Window() = *AOI;

        // Open the subwin without context.
        wdesc->SetContents(MainWdesc());
        wdesc->SetTopCellName(wdesc->CurCellName());

        if (type != WDcddb)
            wdesc->SetSpecial(type, sdb_name, sdb_free);
        if (dspPkgIf()->SubwinInit(i)) {
            wdesc->ShowTitle();
            return (i);
        }
        delete wdesc;
        d_windows[i] = 0;
        show_message("Internal error, could not create new window.", true);
        return (-1);
    }
    // all windows in use, comandeer highest index
    wdesc = d_windows[DSP_NUMWINS-1];
    if (!wdesc->IsSimilar(MainWdesc())) {
        window_mode_change(wdesc);
        wdesc->ClearViews();
    }
    wdesc->WinStr()->eset = false;
    wdesc->WinStr()->pset = false;
    wdesc->ClearSpecial();

    // Open the subwin without context.
    wdesc->SetContents(MainWdesc());
    wdesc->SetTopCellName(wdesc->CurCellName());

    if (type != WDcddb)
        wdesc->SetSpecial(type, sdb_name, sdb_free);
    wdesc->CenterFullView(AOI);
    wdesc->Redisplay(0);
    wdesc->ShowTitle();
    return (DSP_NUMWINS - 1);
}


int
cDisplay::OpenSubwin(const CDs *sdesc, const hyEnt *ent, bool no_top_sym)
{
    if (!sdesc)
        return (-1);
    int i;
    for (i = 1; i < DSP_NUMWINS; i++) {
        if (!d_windows[i])
            break;
    }
    WindowDesc *wdesc;
    if (i < DSP_NUMWINS) {
        wdesc = new WindowDesc();
        d_windows[i] = wdesc;
        if (MainWdesc())
            *wdesc->Attrib() = *MainWdesc()->Attrib();

        wdesc->SetCurCellName(sdesc->cellname());
        wdesc->SetTopCellName(sdesc->cellname());
        wdesc->SetMode(sdesc->displayMode());
        if (no_top_sym)
            wdesc->Attrib()->set_no_elec_symbolic(true);
        wdesc->SetProxy(ent);

        if (dspPkgIf()->SubwinInit(i)) {
            wdesc->ShowTitle();
            wdesc->UpdateProxy();
            wdesc->CenterFullView();
            return (i);
        }
        delete wdesc;
        d_windows[i] = 0;
        show_message("Internal error, could not create new window.", true);
        return (-1);
    }
    // all windows in use, comandeer highest index
    wdesc = d_windows[DSP_NUMWINS-1];
    if (!wdesc->IsSimilar(MainWdesc())) {
        window_mode_change(wdesc);
        wdesc->ClearViews();
    }
    wdesc->WinStr()->eset = false;
    wdesc->WinStr()->pset = false;
    wdesc->ClearSpecial();

    wdesc->SetCurCellName(sdesc->cellname());
    wdesc->SetTopCellName(sdesc->cellname());
    wdesc->SetMode(sdesc->displayMode());
    if (no_top_sym)
        wdesc->Attrib()->set_no_show_unexpand(Electrical, true);
    wdesc->SetProxy(ent);

    wdesc->CenterFullView();
    wdesc->Redisplay(0);
    wdesc->ShowTitle();
    wdesc->UpdateProxy();
    return (DSP_NUMWINS - 1);
}


// Return the WindowDesc associated with the given id, or 0 if
// the id is bad.  The id is an identifier known to the underlying
// graphics system (such as the X-window).
//
WindowDesc *
cDisplay::Windesc(unsigned long n)
{
    for (int i = 0; i < DSP_NUMWINS; i++) {
        if (d_windows[i] && d_windows[i]->WindowId() == n)
            return (d_windows[i]);
    }
    return (0);
}

