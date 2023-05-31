
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
#include "scedif.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "tech_attr_cx.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "tech.h"
#include "ghost.h"
#include "miscutil/texttf.h"

#include <sys/types.h>
#include <time.h>
#include <string.h>


namespace {
    namespace main_hardcopy {
        // Helper class for hardcopy image generation.
        //
        struct sHcImage
        {
            friend void cMain::HCswitchMode(bool, bool, int);

            sHcImage()
            {
                InGo = false;
                CallReset = false;
                AttribsSaved = false;
                DidCopy = false;
                DriverIndex = -1;
            }

            static bool xichcsetup(bool, int, bool, GRdraw*);
            static int xichcgo(HCorientFlags, HClegType, GRdraw*);
            static bool xichcframe(HCframeMode, GRobject, int*, int*, int*,
                int*, GRdraw*);

        private:
            void switch_mode(bool, bool, int);
            void save(bool);
            void restore(bool);
            bool set_window(WindowDesc*, GRdraw*, HCorientFlags, HClegType*,
                int*);
            int legend(WindowDesc*, bool);

            BBox TmpWin;

            // these take care of popdown during hardcopy generation
            bool InGo;          // set while creating hardcopy
            bool CallReset;     // set if popdown while creating hardcopy
            bool AttribsSaved;  // Layer attributes swapped to alt state
            bool DidCopy;       // Output was produced

            int DriverIndex;
        };

        sHcImage HCimg;


        // The Frame command.
        //
        struct FrameState : public CmdState
        {
            friend void cMain::HCdrawFrame(int);
            friend void cMain::HCkillFrame();
            friend struct sHcImage;

            FrameState(const char*, const char*);
            virtual ~FrameState();

            void setCaller(GRobject c)      { Caller = c; }

            BBox *get_frame();
            void suppress_frame_draw(bool b) { FrameSuppress = b; }

            void b1down();
            void b1up();
            void esc();

        private:
            void message() { if (Level == 1) PL()->ShowPrompt(msg1);
                else PL()->ShowPrompt(msg2); }

            void SetLevel1(bool show) { Level = 1; if (show) message(); }
            void SetLevel2() { Level = 2; message(); }

            GRobject Caller;
            int Refx, Refy;
            bool UseB1up;
            bool Pushed;
            static bool FrameOK;        // true when a frame box is defined
            static bool FrameSuppress;  // suppress frame in actual output
            static BBox FrameBox;
            static const char *msg1;
            static const char *msg2;
        };

        FrameState *FrameCmd;
    }
}

using namespace main_hardcopy;

// The FrameCmd struct can go away, but the frame may still be valid
bool FrameState::FrameOK;
bool FrameState::FrameSuppress;
BBox FrameState::FrameBox;
const char *FrameState::msg1 =
    "Click twice or drag to define rectangular area.";
const char *FrameState::msg2 = "Click on second endpoint.";


// Called from cMain constructor.
//
void
cMain::setupHcopy()
{
    Tech()->SetHardcopyFuncs(
        sHcImage::xichcsetup, sHcImage::xichcgo, sHcImage::xichcframe);
}


// Function to switch between the hardcopy attributes and the normal
// attributes.  This is done invisibly if transient is set, during, for
// example, a technology file update in hardcopy mode.  Note that the
// hardcopy parameters are updated before switching back to normal
// mode.
//
void
cMain::HCswitchMode(bool dohc, bool transient, int drvr)
{
    HCimg.switch_mode(dohc, transient, drvr);
}


// Draw a dotted box around the framed area.
//
void
cMain::HCdrawFrame(int display)
{
    if (!FrameState::FrameOK || FrameState::FrameSuppress)
        return;
    WindowDesc *wdesc = DSP()->MainWdesc();
    if (!wdesc->Wdraw())
        return;
    if (display) {
        wdesc->Wdraw()->SetColor(
            DSP()->Color(HighlightingColor, wdesc->Mode()));
    }
    else {
        FrameState::FrameSuppress = true; // avoid reentrancy
        Blist *bl = wdesc->AddEdges(0, &FrameState::FrameBox);
        wdesc->RefreshList(bl);
        Blist::destroy(bl);
        FrameState::FrameSuppress = false;
        return;
    }

    // Draw the whole boundary, otherwise we have to deal with matching
    // the line pattern.
    BBox tBB(*wdesc->ClipRect());
    wdesc->SetClipRect(wdesc->Viewport());
    wdesc->ShowBox(&FrameState::FrameBox, CDL_OUTLINED, 0);
    wdesc->SetClipRect(tBB);

    wdesc->Wdraw()->SetLinestyle(0);
    if (LT()->CurLayer())
        wdesc->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
}


// Destroy the frame definition.
//
void
cMain::HCkillFrame()
{
    if (FrameState::FrameOK) {
        HCdrawFrame(ERASE);
        FrameState::FrameOK = false;
    }
    if (FrameCmd) {
        Menu()->Deselect(FrameCmd->Caller);
        FrameCmd->esc();
    }
}


// Static function.
// Setup callback.  Switch the attributes to/from hardcopy mode,
// set the DSP()->doing_hcopy variable, and redisplay.
//
bool
sHcImage::xichcsetup(bool dohc, int drvr, bool noredisp, GRdraw*)
{
    HCimg.DriverIndex = drvr;
    HCimg.CallReset = false;
    if (dohc) {
        DSP()->SetDoingHcopy(true);
        XM()->HCswitchMode(true, false, drvr);

        // We get here when the user switches drivers in the Print
        // panel, record the change.
        if (drvr > 0)
            Tech()->HC().format = drvr;

        if (!noredisp) {
            LT()->ShowLayerTable();
            DSP()->MainWdesc()->Redisplay(0);
        }
    }
    else {
        if (!DSP()->DoingHcopy())
            return (false);
        if (HCimg.InGo) {
            // popping down, defer mode reset
            HCimg.CallReset = true;
            return (false);
        }
        XM()->HCkillFrame();
        DSP()->SetDoingHcopy(false);
        XM()->HCswitchMode(false, false, drvr);
        if (HCimg.DidCopy)
            *DSP()->MainWdesc()->Window() = HCimg.TmpWin;
        if (!noredisp) {
            LT()->ShowLayerTable();
            DSP()->MainWdesc()->Redisplay(0);
        }
        HCimg.DidCopy = false;
    }
    return (false);
}


// Static function.
// Callback to generate the hardcopy.
//
int
sHcImage::xichcgo(HCorientFlags orient, HClegType showleg, GRdraw*)
{
    bool found_index = false;
    for (int i = 0; DSPpkg::self()->HCof(i); i++) {
        if (HCimg.DriverIndex == i) {
            if (DSP()->MainWdesc()->DbType() == WDchd &&
                    DSPpkg::self()->HCof(i)->line_draw) {
                DSPpkg::self()->HCabort(
                    "Can't use line-draw driver to render CHD image.");
                return (DSPpkg::self()->HCaborted());
            }
            found_index = true;
        }
    }
    if (!found_index) {
        DSPpkg::self()->HCabort("Internal error, bad driver index");
        return (DSPpkg::self()->HCaborted());
    }

    if (!DSP()->CurCellName())
        return (true);
    HCimg.TmpWin = *DSP()->MainWdesc()->Window();
    GRdraw *drawptr = DSPpkg::self()->NewDraw();
    if (!drawptr) {
        HCimg.InGo = false;
        return (true);
    }
    if (drawptr->DevFlags() & GRhasOwnText) {
        // If the driver supports it, use the driver text rendering function
        // for label rendering
        if (!CDvdb()->getVariable(VA_NoDriverLabels))
            DSP()->SetUseDriverLabels(true);
    }
    FrameCmd->suppress_frame_draw(true);
    WindowDesc whc;
    whc.SetContents(DSP()->MainWdesc());
    whc.SetWbag(DSP()->MainWdesc()->Wbag());

    // While in a push, the inverse transform is saved for drawing the
    // context.  The context transform is used for rendering certain
    // objects, such as rulers.  Thus, if the image is rotated (in
    // set_window()), the rotation should also be applied to the stored
    // transform, thus the original is backed up here.
    CDtf tf, ti;
    DSP()->TCurrent(&tf);
    DSP()->TLoad(CDtfRegI0);
    DSP()->TCurrent(&ti);
    DSP()->TLoadCurrent(&tf);
    DSP()->TPush();

    // Set up windows/viewports.  This may turn off legend if there is
    // insufficient space.
    int leg_ht = 0;
    if (HCimg.set_window(&whc, drawptr, orient, &showleg, &leg_ht) &&
            !DSPpkg::self()->HCaborted()) {

        // This is a noop, or dumps the viewport info to the file.
        drawptr->DefineViewport();

        if (!DSPpkg::self()->HCaborted()) {
            // Render the layout.
            WindowDesc *tmpw = DSP()->MainWdesc();
            DSP()->SetWindow(0, &whc);
            whc.RedisplayDirect(0, true, showleg == HClegOn ? leg_ht : 0);
            DSP()->SetWindow(0, tmpw);
        }

        if (showleg == HClegOn)
            HCimg.legend(&whc, false);

        if (!DSPpkg::self()->HCaborted())
            // Dump all accumulated output, or no-op for some drivers.
            drawptr->Dump(DSPpkg::self()->CurDev()->height);
    }

    // put back the stored context transform
    DSP()->TLoadCurrent(&ti);
    DSP()->TStore(CDtfRegI0);

    DSP()->TPop();

    // The Image driver makes use of the frame box, which is a bit bogus
    // due to the scaling for the legend.  Hack this for the callback
    // in Halt().
    BBox BBtmp = FrameState::FrameBox;
    whc.LToPbb(FrameState::FrameBox, FrameState::FrameBox);
    DSP()->MainWdesc()->PToLbb(FrameState::FrameBox,
        FrameState::FrameBox);
    drawptr->Halt();
    FrameState::FrameBox = BBtmp;

    HCimg.DidCopy = !DSPpkg::self()->HCaborted();
    FrameCmd->suppress_frame_draw(false);
    HCimg.InGo = false;
    if (HCimg.CallReset)
        // we've popped down, reset mode
        sHcImage::xichcsetup(false, -1, false, 0);
    DSP()->SetUseDriverLabels(false);

    // Avoid losing the backing pixmap!
    whc.SetWbag(0);

    return (DSPpkg::self()->HCaborted());
}


// Static function.
// This callback allows the user to specify a portion of a cell to
// display in the hardcopy.  The user points or drags to define a
// rectangular area.  A dotted box is drawn when the frame is active.
//
bool
sHcImage::xichcframe(HCframeMode mode, GRobject caller, int *l, int *b,
    int *r, int *t, GRdraw*)
{
    if (mode == HCframeCmd) {
        if (FrameCmd) {
            XM()->HCkillFrame();
            return (false);
        }
        FrameCmd = new FrameState("FRAME", "hcopypanel#frame");
        FrameCmd->setCaller(caller);
        if (!EV()->PushCallback(FrameCmd)) {
            Menu()->Deselect(caller);
            delete FrameCmd;
            FrameCmd = 0;
            return (false);
        }
        FrameCmd->message();
        return (true);
    }
    if (mode == HCframeIsOn)
        return (FrameState::FrameOK);
    if (mode == HCframeOn) {
        bool prev = FrameState::FrameOK;
        FrameState::FrameOK = true;
        return (prev);
    }
    if (mode == HCframeOff) {
        bool prev = FrameState::FrameOK;
        FrameState::FrameOK = false;
        return (prev);
    }
    if (mode == HCframeGet) {
        if (l)
            *l = FrameState::FrameBox.left;
        if (b)
            *b = FrameState::FrameBox.bottom;
        if (r)
            *r = FrameState::FrameBox.right;
        if (t)
            *t = FrameState::FrameBox.top;
        return (FrameState::FrameOK);
    }
    if (mode == HCframeGetV) {
        BBox BB(FrameState::FrameBox);
        DSP()->MainWdesc()->LToPbb(BB, BB);
        if (l)
            *l = BB.left;
        if (b)
            *b = BB.bottom;
        if (r)
            *r = BB.right;
        if (t)
            *t = BB.top;
        return (FrameState::FrameOK);
    }
    if (mode == HCframeSet) {
        if (l)
            FrameState::FrameBox.left = *l;
        if (b)
            FrameState::FrameBox.bottom = *b;
        if (r)
            FrameState::FrameBox.right = *r;
        if (t)
            FrameState::FrameBox.top = *t;
        return (FrameState::FrameOK);
    }
    return (false);
}


void
sHcImage::switch_mode(bool dohc, bool transient, int drvr)
{
    if (dohc) {
        if (Tech()->HcopyDriver() >= 0) {
            if (Tech()->HcopyDriver() == drvr)
                return;
            restore(transient);
        }
        Tech()->SetHcopyDriver(drvr);
        save(transient);
    }
    else {
        restore(transient);
        Tech()->SetHcopyDriver(-1);
    }
    if (!transient)
        Menu()->InitAfterModeSwitch(0);
}


void
sHcImage::save(bool transient)
{
    if (AttribsSaved)
        return;

    DSPattrib *a = DSP()->MainWdesc()->Attrib();
    int drvr = Tech()->HcopyDriver();
    sAttrContext *cx = Tech()->GetAttrContext(drvr, true);
    sAttrContext *cxbak = Tech()->GetHCbakAttrContext();
    *cxbak->attr() = *a;
    *a = *cx->attr();

    // Put back grid resol, snap.  These don't change.
    a->grid(Physical)->set_spacing(
        cxbak->attr()->grid(Physical)->base_spacing());
    a->grid(Physical)->set_snap(
        cxbak->attr()->grid(Physical)->snap());
    a->grid(Electrical)->set_spacing(
        cxbak->attr()->grid(Electrical)->base_spacing());
    a->grid(Electrical)->set_snap(
        cxbak->attr()->grid(Electrical)->snap());

    for (int i = 0; i < AttrColorSize; i++) {
        *cxbak->color(i) = *DSP()->ColorTab()->color_ent(i);
        *DSP()->ColorTab()->color_ent(i) = *cx->color(i);
    }

    CDlgen plgen(Physical);
    CDl *ld;
    while ((ld = plgen.next()) != 0) {
        cxbak->save_layer_attrs(ld);
        cx->restore_layer_attrs(ld);
    }

    CDlgen elgen(Electrical);
    while ((ld = elgen.next()) != 0) {
        // Layers can be in both lists, avoid processing twice.

        if (ld->physIndex() < 0) {
            cxbak->save_layer_attrs(ld);
            cx->restore_layer_attrs(ld);
        }
    }
    AttribsSaved = true;

    if (!transient) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::ALL);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->Wdraw())
                continue;
            if (wdesc->Mode() == DSP()->CurMode()) {
                int pix = DSP()->Color(BackgroundColor);
                wdesc->Wdraw()->SetBackground(pix);
                wdesc->Wdraw()->SetWindowBackground(pix);
            }
        }

        XM()->PopUpColor(0, MODE_UPD);
        XM()->PopUpFillEditor(0, MODE_UPD);
        DSPmainWbag(PopUpGrid(0, MODE_UPD))
        XM()->ShowParameters();
    }
}


void
sHcImage::restore(bool transient)
{
    if (!AttribsSaved)
        return;

    DSPattrib *a = DSP()->MainWdesc()->Attrib();
    int drvr = Tech()->HcopyDriver();
    sAttrContext *cx = Tech()->GetAttrContext(drvr, false);
    sAttrContext *cxbak = Tech()->GetHCbakAttrContext();
    if (cx)
        *cx->attr() = *a;
    *a = *cxbak->attr();
    if (cx) {
        // Keep current grid resol, snap.
        a->grid(Physical)->set_spacing(
            cx->attr()->grid(Physical)->base_spacing());
        a->grid(Physical)->set_snap(
            cx->attr()->grid(Physical)->snap());
        a->grid(Electrical)->set_spacing(
            cx->attr()->grid(Electrical)->base_spacing());
        a->grid(Electrical)->set_snap(
            cx->attr()->grid(Electrical)->snap());
    }

    for (int i = 0; i < AttrColorSize; i++) {
        if (cx)
            *cx->color(i) = *DSP()->ColorTab()->color_ent(i);
        *DSP()->ColorTab()->color_ent(i) = *cxbak->color(i);
    }

    CDlgen plgen(Physical);
    CDl *ld;
    while ((ld = plgen.next()) != 0) {
        if (cx)
            cx->save_layer_attrs(ld);
        cxbak->restore_layer_attrs(ld);
    }

    CDlgen elgen(Electrical);
    while ((ld = elgen.next()) != 0) {
        // Layers can be in both lists, avoid processing twice.

        if (ld->physIndex() < 0) {
            if (cx)
                cx->save_layer_attrs(ld);
            cxbak->restore_layer_attrs(ld);
        }
    }
    AttribsSaved = false;

    if (!transient) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::ALL);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->Wdraw())
                continue;
            if (wdesc->Mode() == DSP()->CurMode()) {
                int pix = DSP()->Color(BackgroundColor);
                wdesc->Wdraw()->SetBackground(pix);
                wdesc->Wdraw()->SetWindowBackground(pix);
            }
        }

        XM()->PopUpColor(0, MODE_UPD);
        XM()->PopUpFillEditor(0, MODE_UPD);
        DSPmainWbag(PopUpGrid(0, MODE_UPD))
        XM()->ShowParameters();
    }
}


// Set up the windows and viewports, and other preliminaries.
//
bool
sHcImage::set_window(WindowDesc *whc, GRdraw *drawptr, HCorientFlags orient,
    HClegType *showleg, int *leg_height)
{
    if (!drawptr)
        return (false);
    whc->SetWdraw(drawptr);
    if (DSP()->MainWdesc())
        *whc->Attrib() = *DSP()->MainWdesc()->Attrib();

    BBox frame;
    if (FrameCmd->get_frame())
        frame = *FrameCmd->get_frame();
    else {
        const BBox *BBp = whc->ContentBB();
        if (BBp) {
            frame = *BBp;
            // expand BB a little
            int dw = frame.width()/40;
            frame.left -= dw;
            frame.right += dw;
            dw = frame.height()/40;
            frame.bottom -= dw;
            frame.top += dw;
        }
        else {
            DSPpkg::self()->HCabort("Window has no content");
            return (false);
        }
    }
    // frame contains the layout area to display.

    int leg_ht = 0;
    int data_w = 0;
    int data_h = 0;
    if (DSPpkg::self()->CurDev()->height == 0) {
        // When height is set to zero, the height is automagically
        // set to the minimum given the width.
        if (DSPpkg::self()->CurDev()->width <= 0) {
            DSPpkg::self()->HCabort("Width too small");
            return (false);
        }
        // In auto-height mode, the legend is always on the bottom.  The
        // cell may be rotated depending on landscape/best fit.  The
        // rotation is done here, not in the driver.

        bool rr = false;
        if (orient & HClandscape)
            rr = true;
        else if (orient & HCbest)
            rr = (frame.width() > frame.height());
        if (rr) {
            // Have to also rotate the stored context transform
            DSP()->TLoad(CDtfRegI0);
            DSP()->TRotate(0, 1);
            DSP()->TStore(CDtfRegI0);
            DSP()->TIdentity();
            DSP()->TRotate(0, 1);
            DSP()->TBB(&frame, 0);
        }

        double af = frame.aspect();
        data_w = frame.width();
        data_h = (int)(data_w/af);

        leg_ht = (*showleg == HClegOn ? legend(whc, true) : 0);
        drawptr->ResetViewport(DSPpkg::self()->CurDev()->width,
            (int)(DSPpkg::self()->CurDev()->width/af) + leg_ht);
    }
    else if (DSPpkg::self()->CurDev()->width == 0) {
        // When width is set to zero, the width is automagically
        // set to the minimum given the height.
        if (DSPpkg::self()->CurDev()->height <= 0) {
            DSPpkg::self()->HCabort("Height too small");
            return (false);
        }

        bool rr = false;
        if (orient & HClandscape)
            rr = true;
        else if (orient & HCbest)
            rr = (frame.width() > frame.height());
        if (rr) {
            // Have to also rotate the stored context transform
            DSP()->TLoad(CDtfRegI0);
            DSP()->TRotate(0, 1);
            DSP()->TStore(CDtfRegI0);
            DSP()->TIdentity();
            DSP()->TRotate(0, 1);
            DSP()->TBB(&frame, 0);
        }

        double af = frame.aspect();
        data_w = frame.width();
        data_h = (int)(data_w/af);

        drawptr->ResetViewport((int)(DSPpkg::self()->CurDev()->height*af),
            DSPpkg::self()->CurDev()->height);
        leg_ht = (*showleg == HClegOn ? legend(whc, true) : 0);
        if (leg_ht > DSPpkg::self()->CurDev()->height/2) {
            leg_ht = 0;
            *showleg = HClegOff;
        }

        // Note that the legend adds to the specified height.
        if (leg_ht)
            drawptr->ResetViewport(DSPpkg::self()->CurDev()->width,
                DSPpkg::self()->CurDev()->height + leg_ht);
    }
    else {
        // The viewport is already specified, this will contain all drawing.
        if (DSPpkg::self()->CurDev()->height <= 0 ||
                DSPpkg::self()->CurDev()->width <= 0) {
            DSPpkg::self()->HCabort("Area too small");
            return (false);
        }
        leg_ht = (*showleg == HClegOn ? legend(whc, true) : 0);
        if (leg_ht > DSPpkg::self()->CurDev()->height/2) {
            leg_ht = 0;
            *showleg = HClegOff;
        }

        double av = ((double)DSPpkg::self()->CurDev()->width)/
            (DSPpkg::self()->CurDev()->height - leg_ht);
        double af = frame.aspect();

        bool rr = false;
        if (orient & HClandscape)
            rr = true;
        else if (orient & HCbest)
            rr = ((av > 1.0 && af < 1.0) || (av < 1.0 && af > 1.0));
        if (rr) {
            // Have to also rotate the stored context transform
            DSP()->TLoad(CDtfRegI0);
            DSP()->TRotate(0, 1);
            DSP()->TStore(CDtfRegI0);
            DSP()->TIdentity();
            DSP()->TRotate(0, 1);
            DSP()->TBB(&frame, 0);
            af = frame.aspect();
        }

        if (af > av) {
            data_w = frame.width();
            data_h = (int)(data_w/av);
        }
        else {
            data_h = frame.height();
            data_w = (int)(data_h*av);
        }
    }

    whc->InitViewport(DSPpkg::self()->CurDev()->width,
        DSPpkg::self()->CurDev()->height);

    int x = (frame.left + frame.right)/2;
    int y = (frame.bottom + frame.top)/2;
    whc->Window()->left = x - data_w/2;
    whc->Window()->right = x + data_w/2;
    whc->Window()->bottom = y - data_h/2;
    whc->Window()->top = y + data_h/2;

    whc->SetRatio(((double)whc->ViewportWidth())/data_w);
    *leg_height = leg_ht;
    return (true);
}


// Show a legend of useful information with the plot.  If nodraw
// is true, simply return the height of the legend.
//
int
sHcImage::legend(WindowDesc *wdesc, bool nodraw)
{
    GRdraw *drawptr = wdesc->Wdraw();
    if (!drawptr)
        return (0);
    int cwidth, fth;
    drawptr->TextExtent(0, &cwidth, &fth);

    int margin = 3*fth/2;
    int entry_ht = 3*fth/2;
    int xspace = entry_ht/2;
    int entry_wd = entry_ht + cwidth*4;
    int yspace = entry_ht/4;

    int num_visible = 0;
    CDl *ld;
    CDlgen lgen(DSP()->CurMode());
    while ((ld = lgen.next()) != 0)
        if (!ld->isInvisible())
            num_visible++;

    int ncols = (DSPpkg::self()->CurDev()->width - 1 + xspace -
        (margin*2))/(entry_wd + xspace);
    if (ncols <= 1)
        // don't show legend
        return (0);
    int nrows = (num_visible - 1)/ncols + 1;

    // We don't draw the layer samples anymore.
    ncols = 1;
    nrows = 0;

    int legheight = (nrows + 1)*(entry_ht + yspace) + 3*yspace + fth;
    if (nodraw)
        return (legheight);

    int maxy = DSPpkg::self()->CurDev()->height - 1;
    int ypos = maxy - (legheight - yspace - fth);

    const BBox *vp = &wdesc->Viewport();
    BBox *wd = wdesc->Window();
    DSPattrib *a = wdesc->Attrib();

    char buf1[80];
    drawptr->SetColor(DSP()->Color(HighlightingColor));
    if (DSP()->CurMode() == Electrical) {
        snprintf(buf1, sizeof(buf1), "circuit: %s",
            Tstring(DSP()->CurCellName()));
    }
    else {
        snprintf(buf1, sizeof(buf1),  "cell: %s",
            Tstring(DSP()->CurCellName()));
    }
    drawptr->Text(buf1, vp->left + cwidth, ypos, 0);
    if (DSP()->CurMode() == Physical) {
        char *s = buf1;
        if (a->grid(wdesc->Mode())->displayed()) {
            double spa = a->grid(Physical)->spacing(Physical);
            int snap = a->grid(Physical)->snap();
            if (snap < 0)
                spa /= -snap;
            else
                spa *= snap;
            snprintf(buf1, sizeof(buf1), "grid: %g  ", spa);
            s = buf1 + strlen(buf1);
        }
        int len = strlen(s);
        snprintf(s, sizeof(buf1) - len, "frame: w=%g h=%g um",
            MICRONS(wd->width()), MICRONS(wd->height()));
        drawptr->Text(buf1, vp->right - cwidth, ypos, TXTF_HJR);
    }
    ypos += entry_ht;

    snprintf(buf1, sizeof(buf1), "Whiteley Research Inc. %s-%s",
        XM()->Product(), XM()->VersionString());
    drawptr->Text(buf1, vp->left + cwidth, ypos, 0);

    time_t secs;
    time(&secs);
    tm *t = localtime(&secs);
    snprintf(buf1, sizeof(buf1), "%02d-%02d-%02d %02d:%02d", t->tm_mon+1,
        t->tm_mday, t->tm_year%100, t->tm_hour, t->tm_min);
    drawptr->Text(buf1, vp->right - cwidth, ypos, TXTF_HJR);

    ypos += 2*yspace;
    drawptr->SetFillpattern(0);
    drawptr->SetLinestyle(0);
    drawptr->Line(vp->left, ypos, vp->right, ypos);
    drawptr->Line(vp->left, vp->bottom, vp->right, vp->bottom);
    drawptr->Line(vp->right, vp->bottom, vp->right, vp->top);
    drawptr->Line(vp->right, vp->top, vp->left, vp->top);
    drawptr->Line(vp->left, vp->top, vp->left, vp->bottom);
    ypos += entry_ht + yspace;

    /* -------------
       Skip drawing the layer samples, requires too much space and
       the name field is assumed to be four characters.

    lgen = CDlgen(DSP()->CurMode());
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            ld = lgen.next();
            if (!ld)
                return (0);
            while (ld->isInvisible()) {
                ld = lgen.next();
                if (!ld)
                    return (0);
            }
            drawptr->SetColor(dsp_prm(ld)->pixel());

            int xpos = vp->left + margin + j*(entry_wd + xspace);

            int xu = xpos + entry_ht;
            int yu = ypos - entry_ht;

            if (!ld->isFilled() || ld->isOutlined()) {
                drawptr->SetFillpattern(0);
                drawptr->SetLinestyle(0);
                drawptr->Line(xpos, ypos, xpos, yu);
                drawptr->Line(xpos, yu, xu, yu);
                drawptr->Line(xu, yu, xu, ypos);
                drawptr->Line(xu, ypos, xpos, ypos);
            }
            if (ld->isFilled()) {
                drawptr->SetFillpattern(dsp_prm(ld)->fill());
                drawptr->Box(xpos, ypos, xu, yu);
            }
            char lname[4+1];
            strncpy(lname, ld->name(), 4);
            lname[4] = '\0';
            xu += cwidth/3;
            drawptr->SetFillpattern(0);
            drawptr->Text(lname, xu, ypos, 0);
        }
        ypos += entry_ht + yspace;
    }
    ---------------- */
    return (0);
}
// End of sHcImage functions.


FrameState::FrameState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Refx = Refy = 0;
    UseB1up = false;
    Pushed = true;
    SetLevel1(false);
}


FrameState::~FrameState()
{
    FrameCmd = 0;
}


// This sets the frame window for the application
//
BBox *
FrameState::get_frame()
{
    return (FrameOK ? &FrameBox : 0);
}


void
FrameState::b1down()
{
    if (Level == 1) {
        // Show ghost box for frame.
        //
        EV()->Cursor().get_raw(&Refx, &Refy);
        Gst()->SetGhost(GFbox_ns);
    }
    else {
        // Finish.
        //
        Gst()->SetGhost(GFnone);
        UseB1up = false;
        WindowDesc *wdesc =
            EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();
        int xr, yr;
        EV()->Cursor().get_raw(&xr, &yr);
        int x = (int)((xr - Refx)*wdesc->Ratio());
        int y = (int)((yr - Refy)*wdesc->Ratio());
        if (x*x >= 64 && y*y >= 64) {
            // min frame is 8x8 pixels
            FrameBox.left = mmMin(xr, Refx);
            FrameBox.right  = mmMax(xr, Refx);
            FrameBox.bottom = mmMin(yr, Refy);
            FrameBox.top = mmMax(yr, Refy);
            PL()->ShowPromptV("Frame entered: %g,%g  %g,%g",
                MICRONS(FrameBox.left), MICRONS(FrameBox.bottom),
                MICRONS(FrameBox.right), MICRONS(FrameBox.top));
            UseB1up = true;
        }
        else
            PL()->ShowPrompt("Frame too small.");
    }
}


// Finish if the pointer moved, otherwise wait for next button 1
// press.
//
void
FrameState::b1up()
{
    if (Level == 1) {
        if (EV()->Cursor().is_release_ok()) {
            WindowDesc *wdesc = EV()->CurrentWin() ?
                EV()->CurrentWin() : DSP()->MainWdesc();
            int xr, yr;
            EV()->Cursor().get_release(&xr, &yr);
            int x = (int)((xr - Refx)*wdesc->Ratio());
            int y = (int)((yr - Refy)*wdesc->Ratio());
            if (x*x >= 64 && y*y >= 64) {
                // min frame is 8x8 pixels
                Gst()->SetGhost(GFnone);
                FrameBox.left = mmMin(xr, Refx);
                FrameBox.right  = mmMax(xr, Refx);
                FrameBox.bottom = mmMin(yr, Refy);
                FrameBox.top = mmMax(yr, Refy);
                PL()->ShowPromptV("Frame entered: %g,%g  %g,%g",
                    MICRONS(FrameBox.left), MICRONS(FrameBox.bottom),
                    MICRONS(FrameBox.right), MICRONS(FrameBox.top));
                EV()->PopCallback(this);
                Pushed = false;
                FrameOK = true;
                XM()->HCdrawFrame(DISPLAY);
                return;
            }
        }
        SetLevel2();
    }
    else {
        // Exit if success, otherwise start over.
        //
        if (UseB1up) {
            EV()->PopCallback(this);
            Pushed = false;
            FrameOK = true;
            XM()->HCdrawFrame(DISPLAY);
        }
        else {
            SetLevel1(true);
        }
    }
}


// Esc, quit.
//
void
FrameState::esc()
{
    PL()->ErasePrompt();
    if (Pushed) {
        EV()->PopCallback(this);
        Gst()->SetGhost(GFnone);
    }
    Menu()->Deselect(Caller);
    delete this;
}

