
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
#include "ext_extract.h"
#include "edit.h"
#include "cd_netname.h"
#include "dsp_tkif.h"
#include "dsp_color.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "events.h"
#include "layertab.h"
#include "tech.h"
#include "tech_layer.h"
#include "promptline.h"
#include "undolist.h"
#include "errorlog.h"
#include "ghost.h"
#include "menu.h"


namespace {
    namespace ext_devsel {
        struct DvselState : public CmdState
        {
            DvselState(const char*, const char*);
            virtual ~DvselState();

            void setCaller(GRobject c)  { Caller = c; }

            void halt();

            void b1down();
            void b1up();
            void b1up_altw();
            void esc();

        private:
            void handle_selection(const BBox*);

            GRobject Caller;
            int Refx, Refy;
            bool B1Dn;
        };

        DvselState *DvselCmd;
    }
}

using namespace ext_devsel;


bool
cExt::selectDevices(GRobject caller)
{
    if (DvselCmd) {
        if (!caller || !MainMenu()->GetStatus(caller))
            DvselCmd->esc();
        return (true);
    }
    else if (caller && !MainMenu()->GetStatus(caller))
        return (true);

    if (!XM()->CheckCurCell(false, true, Physical))
        return (false);

    EV()->InitCallback();
    if (!EX()->extract(CurCell(Physical))) {
        PL()->ShowPrompt("Extraction failed!");
        return (false);
    }

    // Associate for duality, not an error if this fails, but electrical
    // dual selections and comparison won't work.
    if (!EX()->associate(CurCell(Physical)))
        PL()->ShowPrompt("Association failed!");

    DvselCmd = new DvselState("DEVSEL", "xic:dvsel");
    DvselCmd->setCaller(caller);
    if (!EV()->PushCallback(DvselCmd)) {
        delete DvselCmd;
        return (false);
    }
    PL()->ShowPrompt("Click on devices to select.");
    return (true);
}


DvselState::DvselState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Refx = Refy = 0;
    B1Dn = false;
}


DvselState::~DvselState()
{
    DvselCmd = 0;
}


// Bail out of the command, called when clearing groups.  Like the esc()
// method, but don't clear the prompt line.
//
void
DvselState::halt()
{
    if (B1Dn) {
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
    }
    EX()->queueDevice(0);
    EV()->PopCallback(this);
    MainMenu()->Deselect(Caller);
    EX()->PopUpDevices(0, MODE_UPD);
    delete this;
}


// Anchor corner of box and start ghosting.
//
void
DvselState::b1down()
{
    EV()->Cursor().get_raw(&Refx, &Refy);
    XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
    Gst()->SetGhostAt(GFbox, Refx, Refy);
    EV()->DownTimer(GFbox);
    B1Dn = true;
}


// If the user clicks, call the extraction function.  If a drag,
// process the rectangle.  If pressed and held, look for the next
// press.
//
void
DvselState::b1up()
{
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    B1Dn = false;
    if (EV()->Cursor().is_release_ok() && EV()->CurrentWin()) {
        EX()->queueDevice(0);
        int x, y;
        EV()->Cursor().get_release(&x, &y);

        BBox AOI(Refx, Refy, x, y);
        AOI.fix();;
        if (AOI.left == AOI.right && AOI.bottom == AOI.top) {
            int delta = 1 +
                (int)(DSP()->PixelDelta()/EV()->CurrentWin()->Ratio());
            AOI.bloat(delta);
        }
        handle_selection(&AOI);
    }
}


void
DvselState::b1up_altw()
{
    if (EV()->CurrentWin()->CurCellName() != DSP()->MainWdesc()->CurCellName())
        return;
    BBox AOI;
    EV()->Cursor().get_alt_down(&AOI.left, &AOI.bottom);
    EV()->Cursor().get_alt_up(&AOI.right, &AOI.top);
    AOI.fix();
    if (AOI.left == AOI.right && AOI.bottom == AOI.top) {
        int delta = 1 + (int)(DSP()->PixelDelta()/EV()->CurrentWin()->Ratio());
        AOI.bloat(delta);
    }
    handle_selection(&AOI);
}


// Esc entered, clean up and abort.
//
void
DvselState::esc()
{
    if (B1Dn) {
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
    }
    EX()->queueDevice(0);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    MainMenu()->Deselect(Caller);
    EX()->PopUpDevices(0, MODE_UPD);
    delete this;
}


namespace {
    void
    measure(sDevInst *di, const BBox *AOI, double *area, double *cxdist)
    {
        *area = di->bBB()->width();
        *area *= di->bBB()->height();

        double cxd = CDinfinity;
        for (sDevContactInst *c = di->contacts(); c; c = c->next()) {
            int dx = ((c->cBB()->left + c->cBB()->right) -
                (AOI->left + AOI->right))/2;
            int dy = ((c->cBB()->bottom + c->cBB()->top) -
                (AOI->bottom + AOI->top))/2;
            double dst = sqrt(dx*(double)dx + dy*(double)dy);
            if (dst < cxd)
                cxd = dst;
        }
        *cxdist = cxd;
    }
}


void
DvselState::handle_selection(const BBox *AOI)
{
    CDcbin cbin(DSP()->CurCellName());
    if (!cbin.phys())
        return;
    cGroupDesc *gd = cbin.phys()->groups();
    if (!gd)
        return;

    sDevInst *di = 0;
    if (EV()->CurrentWin()->Mode() == Electrical) {
        if (EV()->CurrentWin()->CurCellDesc(Electrical) != cbin.elec())
            return;  // showing symbolic
        if (cbin.elec()) {
            CDg gdesc;
            gdesc.init_gen(cbin.elec(), CellLayer(), AOI);
            CDc *cdesc;
            while ((cdesc = (CDc*)gdesc.next()) != 0) {
                if (cdesc->isDevice()) {
                    // If cdesc is vectorized, match just the 0'th
                    // element.

                    di = gd->find_dual_dev(cdesc, 0);
                    if (di)
                        break;
                }
            }
        }
    }
    else {
        sDevInstList *dv = gd->find_dev(0, 0, 0, AOI);
        if (dv) {
            // Choose the smaller device, or the one where the AOI is
            // closer to a contact.
            double area, cxd;
            measure(dv->dev, AOI, &area, &cxd);
            di = dv->dev;
            for (sDevInstList *d = dv->next; d; d = d->next) {
                double a, c;
                measure(d->dev, AOI, &a, &c);
                if (a < .75 * area) {
                    area = a;
                    cxd = c;
                    di = d->dev;
                }
                else if (a < 1.25 * area && c < .75 * cxd) {
                    area = a;
                    cxd = c;
                    di = d->dev;
                }
            }
        }
        sDevInstList::destroy(dv);
    }
    if (di) {
        EX()->queueDevice(di);

        if (EX()->isDevselCompute()) {
            char *s;
            bool ret = di->net_line(&s, di->desc()->netline1(), PSPM_physical);
            if (s && *s) {
                char buf[32];
                sLstr lstr;
                lstr.add(s);
                lstr.add(" (");
                int cnt = di->count_sections();
                if (cnt > 1) {
                    snprintf(buf, sizeof(buf), "%d sects, ", cnt);
                    lstr.add(buf);
                }
                for (sDevContactInst *c = di->contacts(); c; c = c->next()) {
                    lstr.add(Tstring(c->cont_name()));
                    lstr.add_c(':');
                    snprintf(buf, sizeof(buf), "%d", c->group());
                    lstr.add(buf);
                    lstr.add_c(c->next() ? ' ' : ')');
                }
                PL()->ShowPrompt(lstr.string());
            }
            else {
                if (ret)
                    PL()->ShowPromptV(
                        "Found instance of %s, no print format specified",
                        di->desc()->name());
                else
                    PL()->ShowPrompt(
                        "Exception occurred, operation aborted.");
            }
            delete [] s;
        }
        if (EX()->isDevselCompare()) {
            char buf[256];

            if (di->dual()) {
                if (EV()->Cursor().get_downstate() & GR_SHIFT_MASK) {
                    if (cbin.elec()) {
                        ED()->ulListCheck("exset", cbin.elec(), false);
                        di->set_properties();
                        ED()->ulCommitChanges();
                    }
                }
                sLstr lstr;
                char *instname = di->dual()->instance_name();
                snprintf(buf, sizeof(buf), "Device %s %d --- %s (%s)\n",
                    TstringNN(di->desc()->name()), di->index(), instname,
                    TstringNN(di->dual()->cdesc()->cellname()));
                delete [] instname;
                lstr.add(buf);
                lstr.add("Parameters:\n");
                di->print_compare(&lstr, 0, 0);
                DSPmainWbag(PopUpInfo(MODE_ON, lstr.string()))
            }
            else {
                snprintf(buf, sizeof(buf), "Device %s %d is not associated.",
                    TstringNN(di->desc()->name()), di->index());
                DSPmainWbag(PopUpInfo(MODE_ON, buf))
            }
        }
        return;
    }
    if (EX()->isDevselCompute() || EX()->isDevselCompare())
        PL()->ShowPrompt("No device found.");
}


//-------------------------------------------------------------------------
// These implement a blinking selection capability for devices.
//-------------------------------------------------------------------------

// If command is active, abort.
//
void
cExt::clearDeviceSelection()
{
    if (DvselCmd)
        DvselCmd->halt();
}


// Set a single selected device, and erase any previous device
// selections.  The selected device is shown blinking.
//
void
cExt::queueDevice(sDevInst *di)
{
    sDevInstList *dv = di ? new sDevInstList(di, 0) : 0;
    queueDevices(dv);
    sDevInstList::destroy(dv);
}


// Select the devices in the list passed, and erase any previous
// selections.  The selected devices are shown blinking.
//
void
cExt::queueDevices(sDevInstList *dv)
{
    if (ext_selected_devices) {
        sDevInstList *tdv = ext_selected_devices;
        ext_selected_devices = 0;
        for (sDevInstList *d = tdv; d; d = d->next) {
            WindowDesc *wd;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wd = wgen.next()) != 0) {
                BBox BB(CDnullBB);
                d->dev->show(wd, &BB);
                wd->Redisplay(&BB);
            }
        }
        sDevInstList::destroy(tdv);
    }
    sDevInstList *de = 0;
    for (sDevInstList *d = dv; d; d = d->next) {
        if (!ext_selected_devices)
            ext_selected_devices = de = new sDevInstList(d->dev, 0);
        else {
            de->next = new sDevInstList(d->dev, 0);
            de = de->next;
        }
    }
}


// Called from the color timer to display the selected devices
//
void
cExt::showSelectedDevices(WindowDesc *wd)
{
    if (!wd->Wdraw())
        return;
    if (ext_selected_devices) {
        wd->Wdraw()->SetColor(wd->Mode() == Physical ?
            DSP()->SelectPixelPhys() : DSP()->SelectPixelElec());
        for (sDevInstList *d = ext_selected_devices; d; d = d->next)
            d->dev->show(wd);
        wd->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
    }
}


//-------------------------------------------------------------------------
// Command to draw a highlighting box, while the electrical
// parameters are measured and reported for the box dimensions.
//-------------------------------------------------------------------------

namespace {
    namespace ext_devsel {
        struct MeasState : public CmdState
        {
            friend bool cExt::measureLayerElectrical(GRobject);
            friend void cExt::paintMeasureBox();

            MeasState(const char*, const char*);
            virtual ~MeasState();

            void setCaller(GRobject c)  { Caller = c; }

            const BBox *active_box() { return (BBactive ? &BBarea : 0); }

        private:
            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();
            bool paint_box();

            void SetLevel1() { Level = 1; }
            void SetLevel2() { Level = 2; }

            GRobject Caller;
            int State;
            int Refx, Refy;
            BBox BBarea;
            bool BBactive;
        };

        MeasState *MeasCmd;
    }
}


bool
cExt::measureLayerElectrical(GRobject caller)
{
    if (MeasCmd) {
        if (!caller || !MainMenu()->GetStatus(caller))
            MeasCmd->esc();
        return (true);
    }
    else if (caller && !MainMenu()->GetStatus(caller))
        return (true);

    if (!XM()->CheckCurCell(false, true, Physical))
        return (false);

    EV()->InitCallback();
    MeasCmd = new MeasState("LYRMEAS", "xic:dvsel");
    MeasCmd->setCaller(caller);
    Ulist()->ListCheck(MeasCmd->StateName, CurCell(Physical), false);
    if (!EV()->PushCallback(MeasCmd)) {
        delete MeasCmd;
        return (false);
    }
    PL()->ShowPrompt(
        "Click/drag to define box for measurement, press 'p' fo paint box.");
    return (true);
}


MeasState::MeasState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    Refx = Refy = 0;
    BBactive = false;
    SetLevel1();
}


MeasState::~MeasState()
{
    MeasCmd = 0;
    if (BBactive)
        DSPmainWbag(PopUpInfo(MODE_OFF, 0))
}


// Anchor corner of box and start ghosting.
//
void
MeasState::b1down()
{
    if (Level == 1) {
        EV()->Cursor().get_xy(&Refx, &Refy);
        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
        Gst()->SetGhostAt(GFmeasbox, Refx, Refy);
        DSP()->MainWdesc()->SetAccumMode(WDaccumStart);
        EV()->DownTimer(GFmeasbox);
    }
    else {
        State = 3;
        Gst()->SetGhost(GFnone);
        DSP()->MainWdesc()->GhostFinalUpdate();
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        EX()->showElectrical(abs(x - Refx), abs(y - Refy));
        EX()->showMeasureBox(0, ERASE);
        BBarea = BBox(x, y, Refx, Refy);
        BBarea.fix();
        BBactive = true;
        EX()->showMeasureBox(0, DISPLAY);
        XM()->SetCoordMode(CO_ABSOLUTE);
        EX()->PopUpDevices(0, MODE_UPD);
    }
}


void
MeasState::b1up()
{
    if (Level == 1) {
        if (EV()->Cursor().is_release_ok() && EV()->CurrentWin()) {
            EX()->queueDevice(0);
            if (!EV()->UpTimer()) {
                int x, y;
                EV()->Cursor().get_release(&x, &y);
                EV()->CurrentWin()->Snap(&x, &y);
                if (x != Refx || y != Refy) {
                    Gst()->SetGhost(GFnone);
                    DSP()->MainWdesc()->GhostFinalUpdate();
                    EX()->showElectrical(abs(x - Refx), abs(y - Refy));
                    EX()->showMeasureBox(0, ERASE);
                    BBarea = BBox(x, y, Refx, Refy);
                    BBarea.fix();
                    BBactive = true;
                    EX()->showMeasureBox(0, DISPLAY);
                    XM()->SetCoordMode(CO_ABSOLUTE);
                    EX()->PopUpDevices(0, MODE_UPD);
                    return;
                }
            }
            SetLevel2();
            return;
        }
    }
    else {
        // If a box was defined, reset the state for the next box.
        //
        if (State == 3) {
            State = 2;
            SetLevel1();
        }
    }
}


// Esc entered, clean up and abort.
//
void
MeasState::esc()
{
    EX()->showMeasureBox(0, ERASE);
    Gst()->SetGhost(GFnone);
    DSP()->MainWdesc()->GhostFinalUpdate();
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    MainMenu()->Deselect(Caller);
    EX()->PopUpDevices(0, MODE_UPD);
    delete this;
}


// Handle entering 'p' to paint measure box with current layer.
//
bool
MeasState::key(int, const char *text, int)
{
    if (text && *text) {
        if (*text == 'p') {
            if (BBactive) {
                if (!paint_box())
                    PL()->ShowPrompt("Painting operation failed!");
            }
            else
                PL()->ShowPrompt("No measure box to paint!");
            return (true);
        }
    }
    return (false);
}


void
MeasState::undo()
{
    if (Level != 1) {
        // Undo the corner anchor.
        //
        Gst()->SetGhost(GFnone);
        DSP()->MainWdesc()->GhostFinalUpdate();
        XM()->SetCoordMode(CO_ABSOLUTE);
        if (State == 2)
            State = 3;
        SetLevel1();
    }
    else
        ED()->ulUndoOperation();
}


// Redo undone corner anchor or operation.
//
void
MeasState::redo()
{
    if (Level == 1) {
        if (State == 1 || State == 3) {
            XM()->SetCoordMode(CO_RELATIVE, Refx, Refy);
            Gst()->SetGhostAt(GFmeasbox, Refx, Refy);
            DSP()->MainWdesc()->SetAccumMode(WDaccumStart);
            State = 2;
            SetLevel2();
        }
        else
            ED()->ulRedoOperation();
    }
    else if (State == 2) {
        Gst()->SetGhost(GFnone);
        DSP()->MainWdesc()->GhostFinalUpdate();
        XM()->SetCoordMode(CO_ABSOLUTE);
        ED()->ulRedoOperation();
        SetLevel1();
    }
}


// Actually create the box.  Returns true on success.
//
bool
MeasState::paint_box()
{
    if (!BBactive)
        return (false);
    CDs *cursd = CurCell(Physical);
    if (!cursd)
        return (false);
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return (false);
    if (BBarea.width() == 0 || BBarea.height() == 0)
        return (false);
    CDo *newbox = cursd->newBox(0, &BBarea, ld, 0);
    if (!newbox) {
        Errs()->add_error("newBox failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (false);
    }
    if (!cursd->mergeBoxOrPoly(newbox, true)) {
        Errs()->add_error("mergeBoxOrPoly failed");
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
    }
    Ulist()->CommitChanges(true);
    return (true);
}
// End of MeasState functions.


void
cExt::paintMeasureBox()
{
    if (MeasCmd && MeasCmd->active_box())
        MeasCmd->paint_box();
}


// Pop of an Info window showing the electrical parameters of the
// current layer applied to the passed rectangle.
//
void
cExt::showElectrical(int width, int height)
{
    CDl *ld = LT()->CurLayer();
    if (!ld) {
        DSPmainWbag(PopUpInfo(MODE_ON, "No current layer!"))
        return;
    }

    double wid = MICRONS(abs(width));
    double hei = MICRONS(abs(height));
    if (wid <= 0 || hei <= 0) {
        DSPmainWbag(PopUpInfo(MODE_ON,  
            "Measure box has zero width or height!"))
        return;
    }

    sLstr lstr;
    lstr.add("Layer: ");
    lstr.add(ld->name());
    lstr.add_c('\n');

    char tbuf[256];
    bool found = false;
    double rsh = cTech::GetLayerRsh(ld);
    if (rsh > 0.0) {
        snprintf(tbuf, sizeof(tbuf), "Resistance: %g (L to R), %g (B to T)\n",
            rsh*wid/hei, rsh*hei/wid);
        lstr.add(tbuf);
        found = true;
    }
    double cpa = tech_prm(ld)->cap_per_area();
    double cpp = tech_prm(ld)->cap_per_perim();
    if (cpa > 0.0 || cpp > 0.0) {
        snprintf(tbuf, sizeof(tbuf), "Capacitance: %g\n",
            cpa*wid*hei + cpp*2.0*(wid+hei));
        lstr.add(tbuf);
        found = true;
    }
    double params[8];
    if (cTech::GetLayerTline(ld, params)) {
        // filled in:
        //   params[0] = linethick();
        //   params[1] = linepenet();
        //   params[2] = gndthick();
        //   params[3] = gndpenet();
        //   params[4] = dielthick();
        //   params[5] = dielconst();
        char c;
        double o[4];
        if (wid > hei) {
            params[6] = hei;
            params[7] = wid;
            c = 'X';
        }
        else {
            params[6] = wid;
            params[7] = hei;
            c = 'Y';
        }
        sline(params, o);
        snprintf(tbuf, sizeof(tbuf), "Along %c: L=%g  C=%g  Z=%g  T=%g\n",
            c, o[0], o[1], o[2], o[3]);
        lstr.add(tbuf);
        found = true;
    }
    if (!found) {
        lstr.add("No resistance, capacitance, or transmission line\n"
            "parameters defined for layer.\n");
    }
    DSPmainWbag(PopUpInfo(MODE_ON, lstr.string()))
}


// Display measure area highlighting box.
//
void
cExt::showMeasureBox(WindowDesc *wdesc, bool d_or_e)
{
    if (!MeasCmd || !MeasCmd->active_box())
        return;
    if (!wdesc) {
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            showMeasureBox(wdesc, d_or_e);
        return;
    }

    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
        return;

    if (d_or_e)
        wdesc->Wdraw()->SetColor(
            DSP()->Color(HighlightingColor, Physical));
    else {
        wdesc->Redisplay(MeasCmd->active_box());
        return;
    }

    const BBox *BB = MeasCmd->active_box();
    wdesc->ShowLineW(BB->left, BB->bottom, BB->left, BB->top);
    wdesc->ShowLineW(BB->left, BB->top, BB->right, BB->top);
    wdesc->ShowLineW(BB->right, BB->top, BB->right, BB->bottom);
    wdesc->ShowLineW(BB->right, BB->bottom, BB->left, BB->bottom);

    if (LT()->CurLayer())
        wdesc->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
}


// Function to show the measurement results on-screen.
//
void
cExtGhost::showGhostMeasure(int map_x, int map_y, int ref_x, int ref_y,
    bool erase)
{
    CDl *ld = LT()->CurLayer();
    if (!ld)
        return;

    double wid = MICRONS(abs(map_x - ref_x));
    double hei = MICRONS(abs(map_y - ref_y));
    if (wid <= 0 || hei <= 0)
        return;

    sLstr lstr;
    char tbuf[256];
    double rsh = cTech::GetLayerRsh(ld);
    if (rsh > 0.0) {
        snprintf(tbuf, sizeof(tbuf), "Resistance: %g (L to R), %g (B to T) ",
            rsh*wid/hei, rsh*hei/wid);
        lstr.add(tbuf);
    }
    double cpa = tech_prm(ld)->cap_per_area();
    double cpp = tech_prm(ld)->cap_per_perim();
    if (cpa > 0.0 || cpp > 0.0) {
        snprintf(tbuf, sizeof(tbuf), "Capacitance: %g ",
            cpa*wid*hei + cpp*2.0*(wid+hei));
        lstr.add(tbuf);
    }
    double params[8];
    if (cTech::GetLayerTline(ld, params)) {
        // filled in:
        //   params[0] = linethick();
        //   params[1] = linepenet();
        //   params[2] = gndthick();
        //   params[3] = gndpenet();
        //   params[4] = dielthick();
        //   params[5] = dielconst();
        char c;
        double o[4];
        if (wid > hei) {
            params[6] = hei;
            params[7] = wid;
            c = 'X';
        }
        else {
            params[6] = wid;
            params[7] = hei;
            c = 'Y';
        }
        sline(params, o);
        snprintf(tbuf, sizeof(tbuf), "Tline along %c: L=%g  C=%g  Z=%g  T=%g",
            c, o[0], o[1], o[2], o[3]);
        lstr.add(tbuf);
    }

    if (lstr.string() && *lstr.string()) {
        int x = 4;
        int y = DSP()->MainWdesc()->ViewportHeight() - 5;
        if (erase) {
            int w = 0, h = 0;
            DSPmainDraw(TextExtent(lstr.string(), &w, &h))
            BBox BB(x, y, x + w, y - h);
            DSP()->MainWdesc()->GhostUpdate(&BB);
        }
        else
            DSPmainDraw(Text(lstr.string(), x, y, 0, 1, 1));
    }
}

