
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
#include "editif.h"
#include "extif.h"
#include "drcif.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_gdsii.h"
#include "layertab.h"
#include "menu.h"
#include "select.h"
#include "tech.h"
#include "tech_kwords.h"
#include "events.h"
#include "cvrt.h"
#include "attr_menu.h"
#include "promptline.h"
#include "errorlog.h"


//
// Layer Table management, etc.
//

// The following electrical mode layer names are called by name:
//  SCED, ETC1, ETC2, NAME, MODL, VALU, PARM, NODE, SPTX
//
// SCED:  Active layer for wires
// ETC1:  General drawing layer
// ETC2:  General drawing layer
// NAME:  Name property labels
// MODL:  Model property labels
// VALU:  Value property labels
// PARM:  Param property labels
// SPTX:  Labels will be included in SPICE text

// Default text field width, chars.
#define TEXT_WIDTH 9

// Define to enable selectable icons below table.
#define WITH_END_ICONS


// This command has a state struct since it is "CMD_NOTSAFE" (due to the
// possibility of removing all layers while in a geometry editing command).
//
namespace {
    namespace main_layertab {
        struct LeState : public CmdState
        {
            LeState(const char*, const char*);
            virtual ~LeState();

            void set_cbs(sLcb *cb)  { cbs = cb; }
            char *layername()       { return (cbs ? cbs->layername() : 0); }
            void desel_remove()     { if (cbs) cbs->desel_rem(); }
            void update(CDll *ll)   { if (cbs) cbs->update(ll); }

            void esc() { if (cbs) cbs->popdown(); }

            void (*editfunc)();

            static void add_dispatch();
            static void rem_dispatch();

        private:
            sLcb *cbs;
        };

        LeState *LeCmd;
    }
}

using namespace main_layertab;


// Menu command to pop up the Layer Editor.
//
void
cMain::EditLtabExec(CmdDesc *cmd)
{
    if (LeCmd) {
        LeCmd->esc();
        return;
    }
    LeCmd = new LeState("EDLYR", "xic:edlyr");
    if (!EV()->PushCallback(LeCmd)) {
        if (cmd)
            Menu()->Deselect(cmd->caller);
        delete LeCmd;
        return;
    }
    LeCmd->set_cbs(XM()->PopUpLayerEditor(cmd ? cmd->caller : 0));
    LeCmd->update(CDldb()->unusedLayers(DSP()->CurMode()));
}


// Set/unset curlayer-only selection mode, updates all indicators.
//
void
cMain::SetLayerSpecificSelections(bool b)
{
    if (b) {
        LT()->SetLayerSelectability(LToff, 0);
        if (LT()->CurLayer())
            LT()->SetLayerSelectability(LTon, LT()->CurLayer());
    }
    else
        LT()->SetLayerSelectability(LTon, 0);
}


// Set/unset layer search up selection mode, updates all indicators.
//
void
cMain::SetLayerSearchUpSelections(bool b)
{
    if (b != Selections.layerSearchUp()) {
        Selections.setLayerSearchUp(b);
        XM()->PopUpSelectControl(0, MODE_UPD);
    }
}


LeState::LeState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    cbs = 0;
    editfunc = 0;
}


LeState::~LeState()
{
    LeCmd = 0;
}


// Static function.
// This is the dispatch function while adding a layer.
//
void
LeState::add_dispatch()
{
    char *newname = LeCmd ? LeCmd->layername() : 0;
    if (!newname || !*newname) {
        delete [] newname;
        return;
    }
    if (!LT()->AddLayer(newname, 0)) {
        Log()->ErrorLogV("layer creation",
            "Layer creation failed:\n%s", Errs()->get_error());
    }
}


// Static function.
// This is the dispatch function while removing layers.
//
void
LeState::rem_dispatch()
{
    if (!LT()->CurLayer())
        return;
    LT()->RemoveLayer(LT()->CurLayer()->name(), DSP()->CurMode());
    bool state;
    if (DSP()->CurMode() == Electrical)
        state = (CDldb()->layer(2, Electrical) != 0);
    else
        state = (CDldb()->layer(1, Physical) != 0);
    if (!state && LeCmd) {
        LeCmd->editfunc = 0;
        LeCmd->desel_remove();
    }
    LT()->InitLayerTable();
    LT()->ShowLayerTable();
}


//-----------------------------------------------------------------------------
// cLayerTab functions

cLayerTab *cLayerTab::instancePtr = 0;

cLayerTab::cLayerTab()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cLayerTab already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    ltc_curlayer = 0;
    ltc_ltab = 0;
    ltc_ltab_frozen = false;

    setupInterface();
}


// Private static error exit.
//
void
cLayerTab::on_null_ptr()
{
    fprintf(stderr, "Singleton class cLayerTab used before instantiated.\n");
    exit(1);
}


void
cLayerTab::SetLayerVisibility(LTstate state, const CDl *ld, bool redraw)
{
    if (ltc_ltab) {
        int ent = ld ? ld->index(DSP()->CurMode()) - 1 : -1;
        ltc_ltab->set_layer_visibility(state, ent, redraw);
    }
}


void
cLayerTab::SetLayerSelectability(LTstate state, const CDl *ld)
{
    if (ltc_ltab) {
        int ent = ld ? ld->index(DSP()->CurMode()) - 1 : -1;
        ltc_ltab->set_layer_selectability(state, ent);
    }
}


void
cLayerTab::ProvisionallySetCurLayer(CDl *ld)
{
    ltc_curlayer = ld;
}


// This takes care of telling the application that the current layer
// has changed.
//
void
cLayerTab::SetCurLayer(CDl *ld)
{
    ProvisionallySetCurLayer(ld);
    if (ltc_ltab)
        ltc_ltab->set_layer();
    if (ltc_curlayer) {
        XM()->PopUpLayerPalette(0, MODE_UPD, false, ld);
        XM()->PopUpColor(0, MODE_UPD);
        XM()->PopUpFillEditor(0, MODE_UPD);
        XM()->PopUpLayerParamEditor(0, MODE_UPD, 0, 0);
        EditIf()->widthCallback();
        DrcIf()->layerChangeCallback();
        ExtIf()->layerChangeCallback();
    }
}


namespace {
    struct SclState *SclCmd;

    struct SclState : public CmdState
    {
        SclState(const char *nm, const char *hk) : CmdState(nm, hk)
            {
                Caller = 0;
                List = 0;
                ListSize = 0;
                Index = 0;
                Lastx = 0;
                Lasty = 0;
                TimerId = 0;
            }

        ~SclState()
            {
                SclCmd = 0;
                delete [] List;
            }

        void setCaller(GRobject c)  { Caller = c; }

        void b1down()       { cEventHdlr::sel_b1down(); }

        void b1up()
            {
                BBox BB;
                if (!cEventHdlr::sel_b1up(&BB, 0, B1UP_NOSEL))
                    return;

                int x, y;
                EV()->Cursor().get_raw(&x, &y);
                if (!List || abs(x - Lastx) > 2 || abs(y - Lasty) > 2)
                    newlist(x, y);
                Lastx = x;
                Lasty = y;

                CDl *ld = next_layer();
                if (ld)
                    LT()->SetCurLayer(ld);
                if (TimerId)
                    DSPpkg::self()->RemoveTimeoutProc(TimerId);
                TimerId = DSPpkg::self()->RegisterTimeoutProc(3000,
                    esc_timeout, 0);
                if (ld) {
                    PL()->ShowPromptV(
                        "Got %s, click again for different layer.",
                        ld->name());
                }
            }

        void desel()        { cEventHdlr::sel_esc(); }

        void esc()
            {
                cEventHdlr::sel_esc();
                EV()->PopCallback(this);
                Menu()->Deselect(Caller);
                PL()->ErasePrompt();
                delete this;
            }

        void message() { PL()->ShowPrompt(mesg); }

    private:
        CDl *next_layer();
        void newlist(int, int);
        void add_layers(const CDs*, const BBox*, cTfmStack*);

        static int esc_timeout(void*);

        GRobject Caller;
        unsigned char *List;
        int ListSize;
        int Index;
        int Lastx, Lasty;
        int TimerId;

        static const char *mesg;
    };


    CDl *
    SclState::next_layer()
    {
        if (Index < 0)
            Index = ListSize - 1;
        while (Index >= 0) {
            if (List[Index] && Index) {
                CDl *ld = CDldb()->layer(Index, DSP()->CurMode());
                Index--;
                return (ld);
            }
            Index--;
        }
        if (Index < 0)
            Index = ListSize - 1;
        while (Index >= 0) {
            if (List[Index] && Index) {
                CDl *ld = CDldb()->layer(Index, DSP()->CurMode());
                Index--;
                return (ld);
            }
            Index--;
        }
        return (0);
    }


    void
    SclState::newlist(int x, int y)
    {
        if (!List || CDldb()->layersUsed(DSP()->CurMode()) > ListSize) {
            delete [] List;
            ListSize = CDldb()->layersUsed(DSP()->CurMode());
            List = new unsigned char [ListSize];
        }
        memset(List, 0, ListSize*sizeof(unsigned char));
        BBox BB(x, y, x, y);
        WindowDesc *wd = EV()->ButtonWin(false);
        if (wd) {
            int delta = 2.0/wd->Ratio();
            BB.bloat(delta);
        }
        cTfmStack stk;
        add_layers(CurCell(), &BB, &stk);
        Index = -1;
    }


    void
    SclState::add_layers(const CDs *sd, const BBox *AOI, cTfmStack *stk)
    {
        BBox invBB(*AOI);
        stk->TInverse();
        stk->TInverseBB(&invBB, 0);

        DisplayMode mode = DSP()->CurMode();
        CDl *ld;
        CDsLgen lgen(sd);
        while ((ld = lgen.next()) != 0) {
            if (ld->isInvisible())
                continue;
            if (List[ld->index(mode)])
                continue;
            CDg gdesc;
            stk->TInitGen(sd, ld, AOI, &gdesc);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                if (odesc->is_normal() && odesc->intersect(&invBB, true)) {
                    List[odesc->ldesc()->index(mode)] = 1;
                    break;
                }
            }
        }
        CDg gdesc;
        stk->TInitGen(sd, CellLayer(), AOI, &gdesc);
        CDc *cdesc;
        while ((cdesc = (CDc*)gdesc.next()) != 0) {
            if (!cdesc->is_normal())
                continue;
            CDs *msd = cdesc->masterCell();
            if (!msd)
                continue;
            stk->TPush();
            stk->TApplyTransform(cdesc);
            stk->TPremultiply();
            add_layers(msd, AOI, stk);
            stk->TPop();
        }
    }


    int
    SclState::esc_timeout(void*)
    {
        if (SclCmd)
            SclCmd->esc();
        return (false);
    }

    const char *SclState::mesg = "Click on object to set current layer.";
}


// This implements a command whereby clicking on an object will set
// the current layer to the layer of the object.
//
void
cLayerTab::SetCurLayerFromClick(CmdDesc *cmd)
{
    if (SclCmd) {
        SclCmd->esc();
        return;
    }
    Deselector ds(cmd);
    if (!XM()->CheckCurCell(true, false, DSP()->CurMode()))
        return;

    SclCmd = new SclState("SETCL", "xic:setcl");
    SclCmd->setCaller(cmd ? cmd->caller : 0);
    if (!EV()->PushCallback(SclCmd)) {
        delete SclCmd;
        return;
    }
    SclCmd->message();
    ds.clear();
}


// Handle button 1 press, called from layer palette and locally.
//
void
cLayerTab::HandleLayerSelect(CDl *ld)
{
    LT()->ProvisionallySetCurLayer(ld);
    if (LeCmd && LeCmd->editfunc)
        (*LeCmd->editfunc)();  // this may change current layer
    SetCurLayer(CurLayer());
    ShowLayerTable();
}


// Initialize function, called on application startup, or after
// layer table is changed.
//
void
cLayerTab::InitLayerTable()
{
    if (!ltc_ltab_frozen && ltc_ltab) {
        if (!CDldb()->layer(0, DSP()->CurMode()))
            return;
        if (ltc_curlayer) {
            if (DSP()->CurMode() == Physical) {
                if (ltc_curlayer->physIndex() < 0)
                    ltc_curlayer = 0;
            }
            else {
                if (ltc_curlayer->elecIndex() < 0)
                    ltc_curlayer = 0;
            }
        }
        if (!ltc_curlayer)
            ltc_curlayer = CDldb()->layer(1, DSP()->CurMode());
        ltc_ltab->init();
    }
}


// Alter the viewing color of ldesc.  If ldesc is passed as 0,
// set the background color.
//
void
cLayerTab::SetLayerColor(CDl *ldesc, int r, int g, int b)
{
    if (r < 0)
        r = 0;
    else if (r > 255)
        r = 255;
    if (g < 0)
        g = 0;
    else if (g > 255)
        g = 255;
    if (b < 0)
        b = 0;
    else if (b > 255)
        b = 255;

    if (!ldesc)
        ldesc = CDldb()->layer(0, DSP()->CurMode());
    DspLayerParams *lp = dsp_prm(ldesc);
    lp->set_red(r);
    lp->set_green(g);
    lp->set_blue(b);

    if (ltc_ltab) {
        int pix;
        ltc_ltab->define_color(&pix, r, g, b);
        dsp_prm(ldesc)->set_pixel(pix);
        if (ldesc == CDldb()->layer(0, DSP()->CurMode())) {
            ltc_ltab->SetBackground(pix);
            ltc_ltab->SetWindowBackground(pix);
            WindowDesc *wdesc;
            WDgen wgen(WDgen::MAIN, WDgen::ALL);
            while ((wdesc = wgen.next()) != 0) {
                if (!wdesc->Wdraw())
                    continue;
                if (wdesc->IsSimilar(DSP()->MainWdesc())) {
                    wdesc->Wdraw()->SetBackground(pix);
                    wdesc->Wdraw()->SetWindowBackground(pix);
                }
            }
        }
    }
}


// Return the RGB currently associated with the layer.
//
void
cLayerTab::GetLayerColor(CDl *ldesc, int *r, int *g, int *b)
{
    if (!ldesc)
        ldesc = CDldb()->layer(0, DSP()->CurMode());
    DspLayerParams *lp = dsp_prm(ldesc);
    *r = lp->red();
    *g = lp->green();
    *b = lp->blue();
}


// Paint the layer table entries in the viewing area.  If a CDl
// is passed, overwrite that entry, if visible (for color change with
// static color map).
//
void
cLayerTab::ShowLayerTable(CDl *lx)
{
    if (!ltc_ltab_frozen && ltc_ltab)
        ltc_ltab->show(lx);
}


// Suppress layer table redisplay (pass true).  This can be done when
// we expect a lot of layers to be added, such as when reading the
// tech file.
//
void
cLayerTab::FreezeLayerTable(bool freez)
{
    ltc_ltab_frozen = freez;
    if (!freez) {
        InitLayerTable();
        ShowLayerTable();
    }
}


// Add a new or removed layer to the table at position ix.  If ix is
// 0, use the position of the current layer.  If ix is negative, use
// the top of the layer table.
//
bool
cLayerTab::AddLayer(const char *newname, int ix)
{
    if (!newname || !*newname) {
        Errs()->add_error("null or empty layer name");
        return (false);
    }
    if (ix == 0 && LT()->CurLayer())
        ix = LT()->CurLayer()->index(DSP()->CurMode());
    if (ix < 1)
        ix = CDldb()->layersUsed(DSP()->CurMode());
    CDl *ld = CDldb()->getUnusedLayer(newname, DSP()->CurMode());
    if (ld) {
        if (!CDldb()->insertLayer(ld, DSP()->CurMode(), ix)) {
            // Hmmm, something went wrong, silently ignore this
            // and try to create a new layer.
            Errs()->get_error();
            delete ld;
            ld = 0;
        }
    }
    if (!ld) {
        ld = CDldb()->addNewLayer(newname, DSP()->CurMode(), CDLnormal, ix);
        if (!ld)
            return (false);
    }

    if (ld)
        LT()->ProvisionallySetCurLayer(ld);
    LT()->InitLayerTable();
    LT()->ShowLayerTable();
    XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
    return (true);
}


// Remove the layers listed in s, and add them to the "gonelist".  Return
// a message if error.
//
const char *
cLayerTab::RemoveLayer(const char *s, DisplayMode mode)
{
    if (!s || !*s)
        return ("No layer name given to remove.");

    // first do some checking
    const char *t = s;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        CDl *ld = CDldb()->findLayer(tok, mode);
        delete [] tok;
        if (!ld)
            return ("Layer not found.");
        if (ld->index(mode) == 0)
            return ("Can't remove internal cell layer.");
    }

    s = t;
    while ((tok = lstring::gettok(&s)) != 0) {
        CDl *ld = CDldb()->findLayer(tok, mode);
        int ix = ld->index(mode);
        CDldb()->removeLayer(ix, mode);
        CDldb()->pushUnusedLayer(ld, mode);
        XM()->PopUpLayerPalette(0, MODE_UPD, false, ld);
        Selections.deselectAllLayer(ld);

        if (ltc_curlayer == ld) {
            SetCurLayer(0);
            while (ix > 0) {
                if (CDldb()->layer(ix, DSP()->CurMode())) {
                    SetCurLayer(CDldb()->layer(ix, DSP()->CurMode()));
                    break;
                }
                ix--;
            }
        }
    }
    if (mode == DSP()->CurMode()) {
        InitLayerTable();
        ShowLayerTable();
        XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
    }
    return (0);
}


// Rename the given layer.
//
bool
cLayerTab::RenameLayer(const char *oldname, const char *newname)
{
    if (!CDldb()->renameLayer(oldname, newname))
        return (false);

    InitLayerTable();
    ShowLayerTable();
    return (true);
}


// Create the standard electrical-mode layers, set the ELECSTD flag. 
// This simply marks the layer as one of the predefined electrical
// layers.
//
void
cLayerTab::InitElecLayers()
{
    CDl *ld = CDldb()->newLayer("SCED", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(225, 225, 225);
        ld->addStrmOut(1, 0);
        ld->setElecStd(true);
        // The SCED layer fills polys by default, other layers
        // default to outline.
        ld->setFilled(true);
        ld->setWireActive(true);
    }
    ld = CDldb()->newLayer("ETC1", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(125, 225, 225);
        ld->addStrmOut(2, 0);
        ld->setElecStd(true);
    }
    ld = CDldb()->newLayer("ETC2", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(225, 225, 125);
        ld->addStrmOut(3, 0);
        ld->setElecStd(true);
    }
    ld = CDldb()->newLayer("NAME", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(255, 230, 175);
        ld->addStrmOut(4, 0);
        ld->setElecStd(true);
    }
    ld = CDldb()->newLayer("MODL", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(175, 225, 175);
        ld->addStrmOut(5, 0);
        ld->setElecStd(true);
    }
    ld = CDldb()->newLayer("VALU", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(255, 225, 225);
        ld->addStrmOut(6, 0);
        ld->setElecStd(true);
    }
    ld = CDldb()->newLayer("PARM", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(200, 175, 225);
        ld->addStrmOut(7, 0);
        ld->setElecStd(true);
    }
    ld = CDldb()->newLayer("NODE", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(255, 255, 225);
        ld->addStrmOut(8, 0);
        ld->setElecStd(true);
    }
    ld = CDldb()->newLayer("SPTX", Electrical);
    if (ld) {
        dsp_prm(ld)->setColor(255, 134, 170);
        ld->addStrmOut(10, 0);
        ld->setElecStd(true);
    }
}


CDl *
cLayerTab::LayerAt(int x, int y)
{
    if (!ltc_ltab)
        return (0);
    int entry = ltc_ltab->entry_of_xy(x, y);
    if (entry > ltc_ltab->last_entry()) {
        // Clicked past end of entries.
        return (0);
    }
    return (CDldb()->layer(entry + ltc_ltab->first_visible() + 1,
        DSP()->CurMode()));
}


//-----------------------------------------------------------------------------
// cLtab (layer table display management) methods

cLtab::cLtab()
{
    lt_win_width = 0;
    lt_win_height = 0;
    lt_first_visible = 0;
    lt_vis_entries = 0;
    lt_x_margin = 0;
    lt_spa = 2;
    lt_y_text_fudge = 0;
    lt_box_dimension = 0;
    lt_backg = 0;
    lt_drag_x = 0;
    lt_drag_y = 0;
    lt_dragging = false;
    lt_disabled = false;
    lt_no_phys_redraw = false;
}


// Make sure that we always keep enough space at the bottom for the
// global visibility/selection icons.
#define SPACE_FOR_ICONS 16

// Initialize function, called on application startup, or after
// layer table is changed.
//
void
cLtab::init()
{
    if (lt_disabled)
        return;
    setup_drawable();
    lt_backg = DSP()->Color(BackgroundColor);
    update_scrollbar();
    entry_size(0, 0);
    lt_vis_entries = (lt_win_height - SPACE_FOR_ICONS - 2*lt_spa)/
        (lt_box_dimension + 2*lt_spa + 1);
    SetWindowBackground(DSP()->Color(PromptBackgroundColor));
    SetBackground(DSP()->Color(PromptBackgroundColor));
    set_layer();
}


// Paint the layer table entries.  If a CDl is passed, overwrite that
// entry, if visible (for color change with static color map).  It is
// called by show() from graphics code.
//
void
cLtab::show_direct(const CDl *lx)
{
    if (!CDldb()->layer(0, DSP()->CurMode()))
        return;
    if (lt_backg != DSP()->Color(BackgroundColor))
        lx = 0;
    if (!lx) {
        // Note that Clear() doesn't work with pixmaps.
        SetFillpattern(0);
        SetLinestyle(0);
        SetColor(DSP()->Color(PromptBackgroundColor));
        Box(0, 0, lt_win_width, lt_win_height);
    }
    for (int j = 0; j < lt_vis_entries; j++) {
        int layer = lt_first_visible + j + 1;
        CDl *ld = CDldb()->layer(layer, DSP()->CurMode());
        if (!ld)
            break;

        if (!lx || lx == ld) {
            if (lx) {
                int x, y;
                entry_to_xy(j, &x, &y);
                SetFillpattern(0);
                SetLinestyle(0);
                SetColor(DSP()->Color(PromptBackgroundColor));
                Box(0, y-1, lt_win_width, y + lt_box_dimension + 2);
            }
            box(j, ld);
            indicators(j, ld);
            text(j, ld);
            if (ld == LT()->CurLayer())
                outline(j);
        }
    }
    more();
}


// This toggles the visibility of the layer with the given entry
// index, or all layers if the entry is negative or larger than
// last_entry().  The sample box is not shown for invisible layers. 
// Update the drawing window display if redisplay is true.
//
// In electrical mode, the active layers are always visible.  Instead
// the action on this layer is to turn the fill attribute on/off,
// changing (for now) whether dots are filled or outlined only.
//
void
cLtab::set_layer_visibility(LTstate state, int entry, bool redisplay)
{
    if (entry < 0 || entry > last_entry()) {
        // Set visibility of all layers.
        bool was_off = (state == LToff);
        if (state == LTtoggle) {
            int vis = 0;
            int invis = 0;
            CDl *ld;
            CDlgen lgen(DSP()->CurMode());
            while ((ld = lgen.next()) != 0) {

                // Imported Cadence electrical layers will probably
                // also be in the physical table.  Skip these in
                // physical mode to avoid making imported schematics
                // invisible.

                if (DSP()->CurMode() == Physical) {
                    if (ld->elecIndex() > 0)
                        continue;
                }

                if (DSP()->CurMode() == Electrical && ld->isWireActive())
                    continue;
                if (ld->isInvisible())
                    invis++;
                else
                    vis++;
            }
            if (vis < invis)
                state = LTon;
            if (vis > invis)
                state = LToff;
        }

        CDl *ld;
        CDlgen lgen(DSP()->CurMode());
        while ((ld = lgen.next()) != 0) {
            if (DSP()->CurMode() == Physical) {
                if (ld->elecIndex() > 0)
                    continue;
            }
            if (DSP()->CurMode() == Electrical && ld->isWireActive())
                continue;
            if (ld == LT()->CurLayer()) {
                // If the caller passed LToff, all visibility
                // will be turned off.  In any other case, the
                // current layer will remain visible.

                ld->setInvisible(was_off);
                continue;
            }
            if (ld->isInvisible()) {
                if ((state == LTon || state == LTtoggle) &&
                        !ld->isRstInvisible())
                    ld->setInvisible(false);
            }
            else {
                if (state == LToff || state == LTtoggle) {
                    ld->setInvisible(true);
                    Selections.deselectAllLayer(ld);
                }
            }
        }
        show();
    }
    else {
        CDl *ld = CDldb()->layer(entry + lt_first_visible + 1,
            DSP()->CurMode());
        if (DSP()->CurMode() == Electrical && ld->isWireActive()) {
            if (ld->isFilled()) {
                if (state == LToff || state == LTtoggle)
                    ld->setFilled(false);
            }
            else {
                if (state == LTon || state == LTtoggle)
                    ld->setFilled(true);
            }
        }
        else {
            if (ld->isInvisible()) {
                if (state == LTon || state == LTtoggle)
                    ld->setInvisible(false);
            }
            else {
                if (state == LToff || state == LTtoggle) {
                    ld->setInvisible(true);
                    Selections.deselectAllLayer(ld);
                }
            }
        }
        box(entry, ld);
        indicators(entry, ld);
        if (ld == LT()->CurLayer())
            outline(entry);
    }
    XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
    if (redisplay)
        DSP()->RedisplayAll();
}


// This toggles the selectability of the layer with the given entry
// index, or all layers if the entry is negative or larger than
// last_entry().  If acting on all layers, the current layer is always
// retained as selectable.
//
// Selected objects on layers that become unselectable are deselected.
//
void
cLtab::set_layer_selectability(LTstate state, int entry)
{
    if (entry < 0 || entry > last_entry()) {
        // Set selectability of all layers.
        bool was_off = (state == LToff);
        if (state == LTtoggle) {
            int sel = 0;
            int nosel = 0;
            CDl *ld;
            CDlgen lgen(DSP()->CurMode());
            while ((ld = lgen.next()) != 0) {
                if (ld->isNoSelect())
                    nosel++;
                else
                    sel++;
            }
            if (sel > nosel)
                state = LToff;
            if (sel < nosel)
                state = LTon;
        }

        CDl *ld;
        CDlgen lgen(DSP()->CurMode());
        while ((ld = lgen.next()) != 0) {
            if (ld == LT()->CurLayer()) {
                // If the caller passed LToff, all selectability
                // will be turned off.  In any other case, the
                // current layer will remain selectable.

                ld->setNoSelect(was_off);
                continue;
            }
            if (ld->isNoSelect()) {
                if ((state == LTon || state == LTtoggle) &&
                        !ld->isRstNoSelect())
                    ld->setNoSelect(false);
            }
            else {
                if (state == LToff || state == LTtoggle) {
                    ld->setNoSelect(true);
                    Selections.deselectAllLayer(ld);
                }
            }
        }
        show();
    }
    else {
        CDl *ld = CDldb()->layer(entry + lt_first_visible + 1,
            DSP()->CurMode());
        if (ld->isNoSelect()) {
            if (state == LTon || state == LTtoggle)
                ld->setNoSelect(false);
        }
        else {
            if (state == LToff || state == LTtoggle) {
                ld->setNoSelect(true);
                Selections.deselectAllLayer(ld);
            }
        }
        indicators(entry, ld);
        text(entry, ld);
        if (ld == LT()->CurLayer())
            outline(entry);
    }
    XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
}


// Button1 press/release action.  Reset the current layer to the one
// pointed to, and perform necessary actions.
//
void
cLtab::b1_handler(int x, int y, int state, bool down)
{
    if (down) {
        int last_ent = last_entry();
        int entry = entry_of_xy(x, y);
        if (entry <= last_ent) {
            if (state & (GR_SHIFT_MASK | GR_CONTROL_MASK)) {
                if (state & GR_SHIFT_MASK)
                    set_layer_visibility(LTtoggle, entry,
                        (DSP()->CurMode() == Electrical ||
                        !lt_no_phys_redraw));
                if (state & GR_CONTROL_MASK)
                    set_layer_selectability(LTtoggle, entry);
                return;
            }
            if (x <= lt_x_margin) {
                int xe, ye;
                entry_to_xy(entry, &xe, &ye);
                if (y < ye + lt_box_dimension/2)
                    set_layer_visibility(LTtoggle, entry,
                        (DSP()->CurMode() == Electrical ||
                        !lt_no_phys_redraw));
                else
                    set_layer_selectability(LTtoggle, entry);
                return;
            }
        }

        // set current layer
        if (entry == last_ent + 1) {
            // Clicked on the active area below all layers, the maximum
            // height of this area is the entry height.

            if (LeCmd && LeCmd->editfunc == LeState::add_dispatch)
                entry = last_ent + 1;
            else {
#ifdef WITH_END_ICONS
                if (x < lt_win_width/2)
                    set_layer_visibility(LTtoggle, entry_of_xy(x, y),
                        (DSP()->CurMode() == Electrical ||
                        (lt_no_phys_redraw && (state & GR_SHIFT_MASK)) ||
                        (!lt_no_phys_redraw && !(state & GR_SHIFT_MASK))));
                else
                    set_layer_selectability(LTtoggle, entry_of_xy(x, y));
#endif
                return;
            }
        }
        LT()->HandleLayerSelect(
            CDldb()->layer(entry + lt_first_visible + 1, DSP()->CurMode()));
        lt_dragging = true;
        lt_drag_x = x;
        lt_drag_y = y;
    }
    else
        lt_dragging = false;
}


// Button2 action.  This toggles the visibility of the layers.  The
// screen is not redrawn after this operation in physical mode unless
// the shift key is pressed while making the selection.
//
//
void
cLtab::b2_handler(int x, int y, int state, bool down)
{
    if (down) {
        int last_ent = last_entry();
        int entry = entry_of_xy(x, y);
        if (x <= lt_x_margin && entry <= last_ent) {
            set_layer_visibility(LTtoggle, entry_of_xy(x, y),
                (DSP()->CurMode() == Electrical) ||
                (lt_no_phys_redraw && (state & GR_SHIFT_MASK)) ||
                (!lt_no_phys_redraw && !(state & GR_SHIFT_MASK)));
            return;
        }
#ifdef WITH_END_ICONS
        if (entry <= last_ent + 1) {
            if (x < lt_win_width/2)
                set_layer_visibility(LTtoggle, entry_of_xy(x, y),
                    (DSP()->CurMode() == Electrical) ||
                    (lt_no_phys_redraw && (state & GR_SHIFT_MASK)) ||
                    (!lt_no_phys_redraw && !(state & GR_SHIFT_MASK)));
            else
                set_layer_selectability(LTtoggle, entry_of_xy(x, y));
        }
#endif
    }
}


// Button3 action.  This toggles blinking of the layer.  When activated,
// a timer function periodically alters the color table entry used by the
// selected layer(s).
//
void
cLtab::b3_handler(int x, int y, int state, bool down)
{
    if (down) {
        bool ctrl = (state & GR_CONTROL_MASK);
        bool shft = (state & GR_SHIFT_MASK);
        if (ctrl || shft) {
            int last_ent = last_entry();
            int entry = entry_of_xy(x, y);
            if (entry > last_ent)
                return;
            LT()->HandleLayerSelect(
                CDldb()->layer(entry + lt_first_visible + 1,
                DSP()->CurMode()));

            if (ctrl && !shft)
                Menu()->MenuButtonPress(MMmain, MenuCOLOR);
            else if (!ctrl && shft)
                Menu()->MenuButtonPress(MMmain, MenuFILL);
            else if (ctrl && shft)
                Menu()->MenuButtonPress(MMmain, MenuLPEDT);
        }
        else {
            int entry = entry_of_xy(x, y);
            if (entry <= last_entry()) {
                CDl *ld = CDldb()->layer(entry + lt_first_visible + 1,
                    DSP()->CurMode());
                blink(ld);
            }
        }
    }
}


// Handle scrolling of the layer table.  Obsolete, a scrollbar is now used
// rather than up/down buttons.
//
void
cLtab::scroll_handler(bool down)
{
    int num = CDldb()->layersUsed(DSP()->CurMode()) - 1;
    if (lt_vis_entries >= num)
        return;
    int ents = num/lt_vis_entries + (num%lt_vis_entries ? 1 : 0);

    int visible_field = lt_first_visible/lt_vis_entries;
    if (down) {
        // down arrow
        visible_field--;
        if (visible_field < 0)
            visible_field = ents - 1;
    }
    else {
        // up arrow
        visible_field++;
        if (visible_field >= ents)
            visible_field = 0;
    }
    lt_first_visible = visible_field*lt_vis_entries;
    show();
}


// Return true if a drag should be started.
//
bool
cLtab::drag_check(int x, int y)
{
    if (lt_dragging && (abs(x - lt_drag_x) > 4 || abs(y - lt_drag_y) > 4)) {
        lt_dragging = false;
        return (true);
    }
    return (false);
}


// Draw a small box using layer attributes.
//
void
cLtab::box(int entry, const CDl *ld)
{
    if (lt_disabled || !ld)
        return;
    int x, y;
    entry_to_xy(entry, &x, &y);

    SetFillpattern(0);
    SetLinestyle(0);
    SetColor(DSP()->Color(BackgroundColor));
    Box(x - lt_spa, y - lt_spa, x + lt_box_dimension + 2*lt_spa - 1,
        y + lt_box_dimension + 2*lt_spa);
    if (ld->isInvisible())
        return;

    SetColor(dsp_prm(ld)->pixel());
    if (ld->isFilled()) {
        SetFillpattern(dsp_prm(ld)->fill());
        Box(x, y, x + lt_box_dimension, y + lt_box_dimension);
        if (dsp_prm(ld)->fill()) {
            SetFillpattern(0);
            if (ld->isOutlined()) {
                SetLinestyle(0);
                GRmultiPt xp(5);
                if (ld->isOutlinedFat()) {
                    xp.assign(0, x+1, y+1);
                    xp.assign(1, x+1, y + lt_box_dimension-1);
                    xp.assign(2, x + lt_box_dimension-1, y +
                        lt_box_dimension-1);
                    xp.assign(3, x + lt_box_dimension-1, y+1);
                    xp.assign(4, x+1, y+1);
                    PolyLine(&xp, 5);
                }
                xp.assign(0, x, y);
                xp.assign(1, x, y + lt_box_dimension);
                xp.assign(2, x + lt_box_dimension, y + lt_box_dimension);
                xp.assign(3, x + lt_box_dimension, y);
                xp.assign(4, x, y);
                PolyLine(&xp, 5);
            }
            if (ld->isCut()) {
                SetLinestyle(0);
                Line(x, y, x + lt_box_dimension, y + lt_box_dimension);
                Line(x, y + lt_box_dimension, x + lt_box_dimension, y);
            }
        }
    }
    else {
        GRmultiPt xp(5);
        SetFillpattern(0);
        if (ld->isOutlined()) {
            if (ld->isOutlinedFat()) {
                SetLinestyle(0);
                xp.assign(0, x+1, y+1);
                xp.assign(1, x+1, y + lt_box_dimension-1);
                xp.assign(2, x + lt_box_dimension-1, y + lt_box_dimension-1);
                xp.assign(3, x + lt_box_dimension-1, y+1);
                xp.assign(4, x+1, y+1);
                PolyLine(&xp, 5);
            }
            else
                SetLinestyle(DSP()->BoxLinestyle());
        }
        else
            SetLinestyle(0);
        xp.assign(0, x, y);
        xp.assign(1, x, y + lt_box_dimension);
        xp.assign(2, x + lt_box_dimension, y + lt_box_dimension);
        xp.assign(3, x + lt_box_dimension, y);
        xp.assign(4, x, y);
        PolyLine(&xp, 5);
        if (ld->isCut()) {
            Line(x, y, x + lt_box_dimension, y + lt_box_dimension);
            Line(x, y + lt_box_dimension, x + lt_box_dimension, y);
        }
    }
}


// Update the visibility and selectability indicators.
//
void
cLtab::indicators(int entry, const CDl *ld)
{
    if (lt_disabled)
        return;
    int x, y;
    entry_to_xy(entry, &x, &y);
    SetFillpattern(0);
    SetLinestyle(0);
    SetColor(DSP()->Color(PromptBackgroundColor));
    Box(0, y, lt_x_margin - lt_spa - 1, y + lt_box_dimension); 

    if (ld->isInvisible()) {
        SetColor(DSP()->Color(GUIcolorNo));
        Text("nv", 1, y + lt_box_dimension/2 - lt_y_text_fudge, 0);
    }
    else {
        SetColor(DSP()->Color(GUIcolorYes));
        Text("v", 2, y + lt_box_dimension/2 - lt_y_text_fudge, 0);
    }
    if (ld->isNoSelect()) {
        SetColor(DSP()->Color(GUIcolorNo));
        Text("ns", 1, y + lt_box_dimension - lt_y_text_fudge, 0);
    }
    else {
        SetColor(DSP()->Color(GUIcolorYes));
        Text("s", 2, y + lt_box_dimension - lt_y_text_fudge, 0);
    }
}


// Label the layer table entry with the layer name.
//
void
cLtab::text(int entry, const CDl *ld)
{
    if (lt_disabled)
        return;
    if (!ld)
        return;
    const char *lname = CDldb()->getOAlayerName(ld->oaLayerNum());
    if (!lname)
        lname = "unknown";
    const char *pname = CDldb()->getOApurposeName(ld->oaPurposeNum());
    if (!pname)
        pname = "drawing";

    int x, y;
    entry_to_xy(entry, &x, &y);
    x += lt_box_dimension + 2*lt_spa;

    SetFillpattern(0);
    SetLinestyle(0);
    if (ld->isNoSelect())
        SetColor(DSP()->Color(GUIcolorDvSl));
    else
        SetColor(DSP()->Color(PromptBackgroundColor));
    Box(x, y, lt_win_width, y + lt_box_dimension);

    x += 2*lt_spa;
    SetColor(DSP()->Color(PromptEditTextColor));
    y += lt_box_dimension/2 + lt_y_text_fudge;
    Text(lname, x, y, 0);

    // If the width is too narrow to show the "drawing" purpose label
    // fully, don't draw it.
    //
    if (lstring::cieq(pname, "drawing")) {
        int tw;
        TextExtent(pname, &tw, 0);
        int dx = lt_win_width - x;
        if (tw > dx)
            return;
    }

    SetColor(DSP()->Color(PromptTextColor));
    y += lt_box_dimension/2;
    Text(pname, x, y, 0);
}


// Draw a large open box around the entry, to indicate current layer.
//
void
cLtab::outline(int entry)
{
    if (lt_disabled)
        return;
    int x, y;
    entry_to_xy(entry, &x, &y);
    SetFillpattern(0);
    SetLinestyle(0);
    SetColor(DSP()->Color(PromptCursorColor));
    GRmultiPt xp(5);
    int dx = lt_win_width - x;
    xp.assign(0, x - lt_spa, y - lt_spa);
    xp.assign(1, xp.at(0).x, xp.at(0).y + lt_box_dimension + 2*lt_spa);
    xp.assign(2, xp.at(0).x + dx, xp.at(1).y);
    xp.assign(3, xp.at(2).x, xp.at(0).y);
    xp.assign(4, xp.at(0).x, xp.at(0).y);
    PolyLine(&xp, 5);
    BBox BB(xp.at(0).x, xp.at(2).y, xp.at(2).x, xp.at(0).y);
    update();
}


// Paint the area outside of the layer entries.
//
void
cLtab::more()
{
    if (lt_disabled)
        return;
    SetLinestyle(0);
    SetFillpattern(0);

#ifdef WITH_END_ICONS
    int eht = lt_box_dimension + 2*lt_spa + 1;
    int y = (1 + last_entry())*eht + 2*lt_spa;
    int ht = lt_win_height - y;
    if (ht > eht)
        ht = eht;

    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);
    int x1 = lt_win_width/2;
    SetColor(DSP()->Color(GUIcolorDvSl));
    y += 2;
    Box(2, y, x1 - 2, y + ht);
    Box(x1 + 2, y, lt_win_width - 4, y + ht);

    if (ht >= fhei) {
        SetColor(DSP()->Color(PromptTextColor));
        Text("vis", x1/2 - fwid - fwid/2, y + (ht + fhei)/2, 0);
        Text("sel", x1 + x1/2 - fwid - fwid/2, y + (ht + fhei)/2, 0);
    }
#endif

    // This is the final drawing operation, so update drawing area.
    update();
}


// Return the entry containing point x,y.
//
int
cLtab::entry_of_xy(int, int y)
{
    if (lt_disabled)
        return (0);
    int entry = (y - lt_spa)/(lt_box_dimension + 2*lt_spa + 1);
    if (entry < 0)
        entry = 0;
    return (entry);
}


// Return the upper-left corner of the layer box for the entry.
//
void
cLtab::entry_to_xy(int entry, int *x, int *y)
{
    if (lt_disabled) {
        *x = 0;
        *y = 0;
        return;
    }
    *y = entry*(lt_box_dimension + 2*lt_spa + 1) + 2*lt_spa;
    *x = lt_x_margin;
}


// This sets up the display field constants from the font metrics, and
// returns the field sizes needed for drawing area size allocation.
//
void
cLtab::entry_size(int *px, int *py)
{
    int fwid, fhei;
    TextExtent(0, &fwid, &fhei);
    win_size(&lt_win_width, &lt_win_height);
    lt_box_dimension = 2*fhei;
    lt_x_margin = 2*fwid + 2*lt_spa;
    if (px)
        *px = lt_x_margin + lt_box_dimension + 5*lt_spa + TEXT_WIDTH*fwid;
    if (py)
        *py = lt_box_dimension + 2*lt_spa + 1;
}


// Obtain the pixel for r,g,b.
//
void
cLtab::define_color(int *pix, int r, int g, int b)
{
    if (lt_disabled)
        return;
    DefineColor(pix, r, g, b);
}


// Find the last entry of the current field containing an entry.
//
int
cLtab::last_entry()
{
    int last_ent = lt_first_visible + lt_vis_entries - 1;
    int num = CDldb()->layersUsed(DSP()->CurMode()) - 2;
    if (last_ent > num)
        last_ent = num;
    return (last_ent - lt_first_visible);
}
// End of cLtab functions


LayerFillData::LayerFillData(const CDl *ld)
{
    d_nx = d_ny = 0;
    d_layernum = -1;
    d_from_layer = false;
    d_from_sample = false;
    d_flags = 0;
    memset(d_data, 0, 128);

    if (!ld)
        return;
    d_from_layer = true;
    d_layernum = ld->index(DSP()->CurMode());

    DspLayerParams *lp = dsp_prm(ld);
    if (ld->isFilled() && !lp->fill()->hasMap()) {
        // solid fill
        d_nx = 8;
        d_ny = 8;
        memset(d_data, 0xff, 8);
    }
    else {
        if (!ld->isFilled()) {
            // empty fill
            d_nx = 8;
            d_ny = 8;
            memset(d_data, 0, 8);
        }
        else {
            // stippled fill
            d_nx = lp->fill()->nX();
            d_ny = lp->fill()->nY();
            unsigned char *map = lp->fill()->newBitmap();
            int bpl = (d_nx + 7)/8;
            memcpy(d_data, map, d_ny*bpl);
            delete [] map;
        }
        if (ld->isOutlined()) {
            d_flags |= LFD_OUTLINE;
            if (ld->isOutlinedFat())
                d_flags |= LFD_FAT;
        }
        if (ld->isCut())
            d_flags |= LFD_CUT;
    }
}
// End of LayerFillData functions


//-----------------------------------------------------------------------------
// Implementation of the Layer Editor

// Static function: update layer editor when unused layer list changes.
//
void
sLcb::check_update(CDll *list, DisplayMode mode)
{
    if (LeCmd && mode == DSP()->CurMode())
        LeCmd->update(list);
}


// Callback from the Dismiss button
//
void
sLcb::quit_cb()
{
    EV()->PopCallback(LeCmd);
    delete LeCmd;
}


// Callback for the Add Layer button
//
void
sLcb::add_cb(bool active)
{
    if (LeCmd)
        LeCmd->editfunc = active ? LeState::add_dispatch : 0;
}


// Callback from the Remove Layer button
//
void
sLcb::rem_cb(bool active)
{
    if (LeCmd)
        LeCmd->editfunc = active ? LeState::rem_dispatch : 0;
}
// End of sLcb functions.

