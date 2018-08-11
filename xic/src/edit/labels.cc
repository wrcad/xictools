
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
#include "scedif.h"
#include "undolist.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_gencif.h"
#include "geo_zlist.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "events.h"
#include "layertab.h"
#include "menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "ghost.h"
#include "miscutil/encode.h"
#include "miscutil/texttf.h"
#include "ginterf/grfont.h"


// Fine zoom factor.  Applying this five times is equivalent to the
// coarse zoom factor which doubles.
//
#define FIFTH_ROOT_TWO 1.148698354997035

// create the logo font;
namespace { GRvecFont LogoFT; }
GRvecFont *cEdit::ed_logofont = &LogoFT;

namespace {
    inline bool str_to_int(int *iret, const char *s)
    {
        if (!s)
            return (false);
        return (sscanf(s, "%d", iret) == 1);
    }


    // Attempt to keep the "pixel" width on some kind of decimal
    // grid.
    inline int quantize(int n)
    {
        int nn = 1;
        while (nn < n)
            nn *= 10;
        nn /= 100;
        if (!nn)
            return (n);
        return ((n/nn)*nn);
    }

    // Logo "pixel" size, in internal units, for override from
    // variable, active when nonzero.
    //
    int LogoPixelSize;

    // Multiplier for PathWidth
    //
    float PWmult[] = { 0.5, 0.75, 1.0, 1.5, 2.0 };

    enum LogoType { LogoNormal, LogoManhattan, LogoPretty };

    namespace ed_labels {
        // States for label and logo commands
        struct LabelState : public CmdState
        {
            friend void cEdit::makeLabelsExec(CmdDesc*);
            friend void cEdit::makeLogoExec(CmdDesc*);
            friend void cEdit::logoUpdate();
            friend void cEdit::createLogo(const char*, int, int, int);
            friend void cEditGhost::showGhostLabel(int, int, int, int);

            LabelState(const char*, const char*);
            virtual ~LabelState();

            void setup(GRobject c, bool dologo)
                {
                    Caller = c;
                    DoLogo = dologo;
                    if (dologo)
                        ED()->PopUpLogo(0, MODE_ON);
                }

            void set_curtx(bool);
            void set_size(double, double);
            void esc();
            void b1down();
            bool key(int, const char*, int);

            void show_logo(WindowDesc*, char*, int, int, int, int, int,
                GRvecFont*);

        private:
            void message()
                {
                    if (DoLogo)
                        PL()->ShowPrompt(msg2);
                    else if (Target)
                        PL()->ShowPrompt(
                            "Click or press Enter to update label.");
                    else if (TrgWire)
                        PL()->ShowPrompt(
                            "Click to place label, will be bound to "
                            "selected wire.");
                    else
                        PL()->ShowPrompt(msg1);
                }
            int current_xform();

            GRobject Caller;
            hyList *Text;
            bool Pushed;
            bool DoLogo;
            LogoType LogoMode;
            int MinPix;
            PolyList *Plist;        // Manhattan font rendering polygon list
            int Xos, Yos, Pwidth;   // Manhattan rendering parameters
            WireStyle Style;        // Wire style for logos
            int PathWidth;          // Wire width for logos
            bool KeepLT;            // Don't remove string editor
            bool TxSaved;           // Current transform was saved
            int TxHJ, TxVJ;         // Saved justification
            CDla *Target;           // Label being updated
            CDw *TrgWire;           // Wire for node label
            struct sCurTx TxBak;    // Current Transform store

            static const char *msg1;
            static const char *msg2;
        };

        LabelState *LabelCmd;
    }
}

using namespace ed_labels;

const char *LabelState::msg1 = "Click on locations to place the label.";
const char *LabelState::msg2 = "Click on locations to place the text.";

// misc. local exports
namespace label {
    void write_logo(const cTfmStack*, CDl*, const char*, int, int, int,
        WireStyle);
    void write_logofile(FILE*, const char*, const char*, int, int, int,
        WireStyle, const char*);
    char *new_filename(const char*, const char*, int, int, int, int);
    PolyList *string_to_polys(const char*, int, int, int, int, bool);
    void manh_text_extent(const char*, int*, int*, int*, bool);
    bool is_xpm(const char*);
    Zlist *xpm_to_zlist(const char*, int, int, int);
    bool xpm_size(const char*, int*, int*);
}


// Note - using LabelCmd for both commands was a mistake.  Have to be
// very careful about the user selecting one command while in the other.

namespace {
    // Examine the labels in the selection list.  The return value is
    // the number of labels found.  If all labels have the same text,
    // p_hstr will point to this text.  If all labels support long text,
    // or there are no labels, p_ltext will point to true;
    //
    int check_labels(CDs *sd, hyList **p_hstr, bool *p_ltext)
    {
        int count = 0;
        hyList *hstr = 0;
        bool ltext = true;
        bool zero_hstr = false;
        sSelGen sg(Selections, sd, "l");
        CDla *la;
        while ((la = (CDla*)sg.next()) != 0) {
            count++;
            hyList *ihstr = 0;
            bool use_lt = false;
            if (la->label() && la->label()->ref_type() == HLrefLongText) {
                use_lt = true;
                // A bound long text label may not have the expanded text,
                // so grab it from the property.
                CDp_lref *prf = (CDp_lref*)la->prpty(P_LABRF);
                if (prf && prf->propref() &&
                        ((CDp_user*)prf->propref())->data())
                    ihstr = ((CDp_user*)prf->propref())->data();
                else
                    ihstr = la->label();
            }
            else {
                if (DSP()->CurMode() == Physical)
                    use_lt = true;
                else {
                    CDp_lref *prf = (CDp_lref*)la->prpty(P_LABRF);
                    if (!prf || !prf->devref() || (prf->propref() &&
                            (prf->propref()->value() == P_VALUE ||
                            prf->propref()->value() == P_PARAM ||
                            prf->propref()->value() == P_OTHER)))
                        use_lt = true;
                }
                ihstr = la->label();
            }

            // Turn off long text if a label doesn't support it.
            if (!use_lt)
                ltext = false;

            // Zero the label string unless all strings have the same text.
            if (!zero_hstr) {
                if (!hstr) {
                    hstr = ihstr;
                    if (!ihstr)
                        zero_hstr = true;
                }
                else {
                    if (!ihstr || hyList::hy_strcmp(hstr, ihstr)) {
                        hstr = 0;
                        zero_hstr = true;
                    }
                }
            }
        }
        if (p_hstr)
            *p_hstr = hstr;
        if (p_ltext)
            *p_ltext = ltext;
        return (count);
    }
}


void
cEdit::makeLabelsExec(CmdDesc *cmd)
{
    const char *lmsg = "Enter text for selected %s: ";
    if (LabelCmd) {
        bool waslogo = LabelCmd->DoLogo;
        LabelCmd->esc();
        if (!waslogo)
            return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;
    LabelCmd = new LabelState("LABEL", "xic:label");
    LabelCmd->setup(cmd ? cmd->caller : 0, false);

    CDs *cursd = CurCell();
    if (!cursd)
        // can't happen
        return;
    hyList *init_htext;
    bool waswire = false;
    bool use_ltext;
    int count = check_labels(cursd, &init_htext, &use_ltext);
    if (count == 0 && DSP()->CurMode() == Electrical) {
        // If there are no labels selected, but exactly one wire on
        // and active layer is selected, the label will be attached to
        // the wire to set the net name.

        sSelGen sg(Selections, cursd, "w");
        CDw *w;
        while ((w = (CDw*)sg.next()) != 0) {
            if (w->ldesc()->isWireActive()) {
                if (LabelCmd->TrgWire) {
                    LabelCmd->TrgWire = 0;
                    break;
                }
                LabelCmd->TrgWire = w;
            }
        }
        if (LabelCmd->TrgWire) {
            // If a label already exists, go to update mode.
            CDp_bnode *pb = (CDp_bnode*)LabelCmd->TrgWire->prpty(P_BNODE);
            if (pb && pb->bound()) {
                LabelCmd->Target = pb->bound();
                LabelCmd->TrgWire = 0;
                count = 1;
                waswire = true;
            }
            if (!waswire) {
                CDp_node *pn = (CDp_node*)LabelCmd->TrgWire->prpty(P_NODE);
                if (pn && pn->bound()) {
                    LabelCmd->Target = pn->bound();
                    LabelCmd->TrgWire = 0;
                    count = 1;
                    waswire = true;
                }
            }
        }
    }

    char buf[256];
    hyList *hlabel;
    if (count) {
        sprintf(buf, lmsg,
            (count == 1) ? (waswire ? "wire" : "label") : "labels");
        hlabel = PL()->EditHypertextPrompt(buf, init_htext, use_ltext);
    }
    else if (LabelCmd->TrgWire) {
        sprintf(buf, lmsg, "wire");
        hlabel = PL()->EditHypertextPrompt(buf, init_htext, use_ltext);
    }
    else
        hlabel = PL()->EditHypertextPrompt("Enter label: ", 0, true);

    if (!hlabel || (hlabel->ref_type() == HLrefText && !hlabel->text()[0])) {
        if (hlabel)
            hyList::destroy(hlabel);
        // careful! might be in the logo command now
        if (LabelCmd && !LabelCmd->DoLogo)
            LabelCmd->esc();
        return;
    }

    // NOTE
    // If viewing a "long text" label, the cell modification count is
    // incremented, whether or not the text is actually changed.  This
    // is because the hyList is actually changed now.

    Ulist()->ListCheck(LabelCmd->StateName, cursd, false);

    // Should the label(s) remain selected after the change?  Presently,
    // if only one is changed, it is deselected, otherwise multiple
    // labels remain selected.

    if (count > 1 || (count == 1 && hlabel->ref_type() == HLrefLongText)) {
        sSelGen sg(Selections, cursd, "l");
        CDla *la;
        while ((la = (CDla*)sg.next()) != 0) {
            CDla *tl = changeLabel(la, cursd, hlabel);
            if (tl) {
                if (count == 1)
                    sg.remove();
                else
                    sg.replace(tl);
            }
        }
        Ulist()->CommitChanges(true);
        hyList::destroy(hlabel);
        LabelCmd->KeepLT = true;
        LabelCmd->esc();
        if (DSP()->CurMode() == Electrical)
            ScedIf()->PopUpNodeMap(0, MODE_UPD);
        XM()->ShowParameters();
        return;
    }
    if (count == 1) {
        // We're updating a single label, in this case attach it to
        // the mouse pointer so it can be resized, etc.  When the user
        // clicks or presses Enter, the existing label will be
        // resized/replaced.

        if (!LabelCmd->Target) {
            sSelGen sg(Selections, cursd, "l");
            LabelCmd->Target = (CDla*)sg.next();
        }
    }

    unsigned int pw;
    int numlines;
    if (LabelCmd->Target) {
        LabelCmd->set_curtx(true);

        GRvecFont *ft = &FT;
        char *text = hyList::string(LabelCmd->Target->label(), HYcvPlain,
            false);
        int width, height;
        ft->textExtent(text, &width, &height, &numlines);
        delete [] text;

        // Match the size, or the closest quantized size if the user
        // has stretched it.
        pw = height/numlines/FT.cellHeight();
    }
    else {
        WindowDesc *wdesc =
            EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();
        pw = (unsigned int)(2.0/wdesc->Ratio());
    }
    if (pw < 2)
        pw = 2;

    // Round the factor to the next higher power of two.  From
    // bit-twiddling hacks
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    //
    // This is an attempt to keep label size quantized, so that size
    // matching can always be achieved with the arrow keys, whatever
    // the zoom factor.
    //
    pw--;
    pw |= pw >> 1;
    pw |= pw >> 2;
    pw |= pw >> 4;
    pw |= pw >> 8;
    pw |= pw >> 16;
    pw++;

    double mw, mh;
    if (DSP()->CurMode() == Physical) {
        mw = MICRONS(pw*FT.cellWidth());
        mh = MICRONS(pw*FT.cellHeight());
    }
    else {
        mw = ELEC_MICRONS(pw*FT.cellWidth());
        mh = ELEC_MICRONS(pw*FT.cellHeight());
    }
    if (LabelCmd->Target) {
        double h;
        if (DSP()->CurMode() == Physical)
            h = MICRONS(LabelCmd->Target->height());
        else
            h = ELEC_MICRONS(LabelCmd->Target->height());
        h /= numlines;

        for (;;) {
            if (mh > h) {
                double mhx = mh/FIFTH_ROOT_TWO;
                if (mhx > h) {
                    mh = mhx;
                    mw /= FIFTH_ROOT_TWO;
                    continue;
                }
                if (mhx <= h) {
                      if (h < mhx * sqrt(FIFTH_ROOT_TWO)) {
                        mh = mhx;
                        mw /= FIFTH_ROOT_TWO;
                    }
                    break;
                }
            }
            else {
                double mhx = mh*FIFTH_ROOT_TWO;
                if (mhx <= h) {
                    mh = mhx;
                    mw *= FIFTH_ROOT_TWO;
                    continue;
                }
                if (mhx > h) {
                    if (h > mh * sqrt(FIFTH_ROOT_TWO)) {
                        mh = mhx;
                        mw *= FIFTH_ROOT_TWO;
                    }
                    break;
                }
            }
        }
    }
    LabelCmd->set_size(mw, mh);

    LabelCmd->Text = hlabel;
    if (!EV()->PushCallback(LabelCmd)) {
        LabelCmd->esc();
        return;
    }
    LabelCmd->Pushed = true;
    LabelCmd->message();
    Gst()->SetGhost(GFlabel);
    ds.clear();
}


// Change the label text, resetting associated properties.  Takes care of
// redisplay and undo list.
//
CDla *
cEdit::changeLabel(CDla *ladesc, CDs *sdesc, hyList *lastr)
{
    if (!sdesc)
        return (0);
    if (!ladesc || ladesc->type() != CDLABEL)
        return (0);
    if (sdesc->isElectrical())
        return (ScedIf()->changeLabel(ladesc, sdesc, lastr));

    const char *label_change = "label change";
    Errs()->init_error();
    Label label(ladesc->la_label());
    if (!labelOverride(&label))
        DSP()->LabelResize(lastr, label.label, &label.width, &label.height);
    label.label = lastr;
    CDla *nlabel = sdesc->newLabel(ladesc, &label, ladesc->ldesc(),
        ladesc->prpty_list(), true);
    if (!nlabel) {
        Errs()->add_error("newLabel failed");
        Log()->ErrorLog(label_change, Errs()->get_error());
        return (0);
    }
    BBox BB = ladesc->oBB();
    BB.add(&nlabel->oBB());
    DSP()->RedisplayArea(&BB);
    return (nlabel);
}


// If the user clicked on an unselected script label, execute the script.
// The script string has the form
//    !!script [name=xxx] [path=xxx] [body]
//
bool
cEdit::execLabelScript()
{
    BBox BB;
    EV()->Cursor().get_raw(&BB.left, &BB.top);
    BB.right = BB.left;
    BB.bottom = BB.top;
    char types[2];
    types[0] = CDLABEL;
    types[1] = 0;
    CDol *slist = Selections.selectItems(CurCell(), types, &BB, PSELpoint);
    if (!slist)
        return (false);
    bool didexec = false;
    for (CDol *sl = slist; sl; sl = sl->next) {
        if (sl->odesc->state() == CDobjSelected)
            continue;
        if (!OLABEL(sl->odesc)->label()->is_label_script())
            continue;

        char *string = hyList::string(OLABEL(sl->odesc)->label(), HYcvPlain,
            true);
        char *path = 0;
        const char *s = string;
        lstring::advtok(&s);
        while (*s) {
            if (lstring::ciprefix("name=", s)) {
                s += 5;
                lstring::advqtok(&s);
                continue;
            }
            else if (lstring::ciprefix("path=", s)) {
                s += 5;
                path = lstring::getqtok(&s);
                continue;
            }
            break;
        }
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->Wdraw())
                continue;
            if (!wdesc->IsSimilar(DSP()->MainWdesc()))
                continue;
            wdesc->Wdraw()->SetColor(
                DSP()->Color(HighlightingColor, wdesc->Mode()));
            Label la(OLABEL(sl->odesc)->la_label());
            wdesc->ShowLabel(&la);
        }
        if (path) {
            SIfile *sfp;
            stringlist *wl;
            XM()->OpenScript(path, &sfp, &wl);
            if (sfp || wl) {
                SI()->Interpret(sfp, wl, 0, 0);
                if (sfp)
                    delete sfp;
            }
        }
        else
            SI()->Interpret(0, 0, &s, 0);
        wgen = WDgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (!wdesc->Wdraw())
                continue;
            if (!wdesc->IsSimilar(DSP()->MainWdesc()))
                continue;
            wdesc->Wdraw()->SetColor(dsp_prm(sl->odesc->ldesc())->pixel());
            Label la(OLABEL(sl->odesc)->la_label());
            wdesc->ShowLabel(&la);
        }
        didexec = true;
        delete [] path;
        delete [] string;
        break;
    }
    return (didexec);
}


void
cEdit::makeLogoExec(CmdDesc *cmd)
{
    if (LabelCmd) {
        bool waslogo = LabelCmd->DoLogo;
        LabelCmd->esc();
        if (waslogo)
            return;
    }
    Deselector ds(cmd);
    if (noEditing())
        return;
    if (!XM()->CheckCurLayer())
        return;
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;

    LabelCmd = new LabelState("LOGO", "xic:logo");
    LabelCmd->setup(cmd ? cmd->caller : 0, true);

    char *s = PL()->EditPrompt("Enter text: ", 0);
    if (s == 0 || !*s) {
        // careful! might be in the label command now
        if (LabelCmd && LabelCmd->DoLogo)
            LabelCmd->esc();
        return;
    }
    char buf[84];
    strncpy(buf, s, 80);
    buf[80] = '\0';

    int dd;
    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoAltFont)) &&
            dd >= 0 && dd <= 1) {
        if (dd == 0)
            LabelCmd->LogoMode = LogoManhattan;
        else
            LabelCmd->LogoMode = LogoPretty;
    }
    else
        LabelCmd->LogoMode = LogoNormal;

    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoEndStyle)) &&
            dd >= 0 && dd <= 2)
        LabelCmd->Style = (WireStyle)dd;
    else
        LabelCmd->Style = (WireStyle)DEF_LOGO_END_STYLE;

    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoPathWidth)) &&
            dd >= 1 && dd <= 5)
        LabelCmd->PathWidth = dd;
    else
        LabelCmd->PathWidth = DEF_LOGO_PATH_WIDTH;

    WindowDesc *wdesc =
        EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();
    int pw = (int)(2.0/wdesc->Ratio());
    if (pw < 2)
        pw = 2;

    double mw = MICRONS(pw*LogoFT.cellWidth());
    double mh = MICRONS(pw*LogoFT.cellHeight());
    LabelCmd->set_size(mw, mh);

    LabelCmd->Text = new hyList(0, buf, HYcvPlain);
    if (!EV()->PushCallback(LabelCmd)) {
        LabelCmd->esc();
        return;
    }
    LabelCmd->Pushed = true;
    LabelCmd->message();
    Gst()->SetGhost(GFlabel);
    ds.clear();
}


// Call this after variable change
//
void
cEdit::logoUpdate()
{
    if (LabelCmd && LabelCmd->DoLogo && LabelCmd->Text) {
        Gst()->SetGhost(GFnone);
        PolyList::destroy(LabelCmd->Plist);
        LabelCmd->Plist = 0;

        int dd;
        if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoAltFont)) &&
                dd >= 0 && dd <= 1) {
            if (dd == 0)
                LabelCmd->LogoMode = LogoManhattan;
            else
                LabelCmd->LogoMode = LogoPretty;
        }
        else
            LabelCmd->LogoMode = LogoNormal;

        if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoPathWidth)) &&
                dd >= 1 && dd <= 5)
            LabelCmd->PathWidth = dd;
        else
            LabelCmd->PathWidth = DEF_LOGO_PATH_WIDTH;

        if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoEndStyle)) &&
                dd >= 0 && dd <= 2)
            LabelCmd->Style = (WireStyle)dd;
        else
            LabelCmd->Style = (WireStyle)DEF_LOGO_END_STYLE;

        Gst()->SetGhost(GFlabel);
    }
}


// This is called by the logo widget update method when the
// LogoPixelSize variable is updated.  It actually changes the display
// size, taking care of the ghosting update.
//
void
cEdit::assert_logo_pixel_size()
{
    int n = 0;
    const char *pwstr = CDvdb()->getVariable(VA_LogoPixelSize);
    if (pwstr) {
        char *nptr;
        double d = strtod(pwstr, &nptr);
        if (nptr != pwstr && d > 0.0 && d <= 100.0)
            n = INTERNAL_UNITS(d);
    }
    if (n != LogoPixelSize) {
        DSPmainDraw(ShowGhost(ERASE))
        LogoPixelSize = n;
        DSPmainDraw(ShowGhost(DISPLAY))
    }
}


// Create the physical text
//
void
cEdit::createLogo(const char *str, int x, int y, int csdim)
{
    if (!LT()->CurLayer())
        return;

    // Maybe override "pixel" size.
    if (LogoPixelSize)
        csdim = LogoPixelSize;

    int dd;
    int pw = DEF_LOGO_PATH_WIDTH;
    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoPathWidth)) &&
            dd >= 1 && dd <= 5)
        pw = dd;

    WireStyle style = (WireStyle)DEF_LOGO_END_STYLE;
    if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoEndStyle)) &&
            dd >= 0 && dd <= 2)
        style = (WireStyle)dd;

    if (!CDvdb()->getVariable(VA_LogoToFile)) {
        // place logo
        CDl *ld = LT()->CurLayer();
        cTfmStack stk;
        stk.TPush();
        GEO()->applyCurTransform(&stk, 0, 0, x, y);
        int pwid = quantize((int)(csdim*PWmult[pw-1]));
        label::write_logo(&stk, ld, str, csdim, csdim, pwid, style);
        stk.TPop();
    }
    else {
        // create logo file
        LogoType ltype = LogoNormal;
        if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoAltFont)) &&
                dd >= 0 && dd <= 1) {
            if (dd == 0)
                ltype = LogoManhattan;
            else
                ltype = LogoPretty;
            style = (WireStyle)0;
            pw = 0;
        }
        char *fname = label::new_filename(str, LT()->CurLayer()->name(),
            csdim, pw, horzJustify(), style);
        FILE *fp = FIO()->POpen(fname, "r");
        bool exists = true;
        if (!fp) {
            exists = false;
            fp = fopen(fname, "w");
            if (fp == 0) {
                PL()->ShowPrompt("Can't open output file.\n");
                delete [] fname;
                return;
            }
            char *docstr = new char[strlen(str) + 64];
            sprintf(docstr,
                "string=\"%s\" layer=%s dim=%d pw=%d hj=%d style=%d",
                str, LT()->CurLayer()->name(), csdim, pw, horzJustify(),
                style);

            int pwid = pw ? quantize((int)(csdim*PWmult[pw-1])) : 0;
            label::write_logofile(fp, fname, str, csdim, csdim, pwid, style,
                docstr);
            delete [] docstr;
        }
        fclose(fp);

        sCurTx txbak = *GEO()->curTx();
        sCurTx txtmp = *GEO()->curTx();
        txtmp.set_magn(1.0);
        GEO()->setCurTx(txtmp);

        int xos = 0, yos = 0;
        if (label::is_xpm(str)) {
            int lwid, lhei;
            if (label::xpm_size(str, &lwid, &lhei)) {
                switch (vertJustify()) {
                case 1:
                    yos += (csdim*lhei)/2;
                    break;
                case 2:
                    yos += csdim*lhei;
                    break;
                }
                switch (horzJustify()) {
                case 1:
                    xos += (csdim*lwid)/2;
                    break;
                case 2:
                    xos += csdim*lwid;
                    break;
                }
            }
            else if (exists) {
                CDcbin cbin;
                FIOreadPrms prms;
                OItype oiret = FIO()->OpenImport(fname, &prms, 0, 0, &cbin);
                if (oiret == OIerror) {
                    GEO()->setCurTx(txbak);
                    delete [] fname;
                    Log()->ErrorLog(mh::Internal,
                        "Internal error: can't open existing cell.");
                    return;
                }
                if (oiret == OIaborted) {
                    GEO()->setCurTx(txbak);
                    delete [] fname;
                    return;
                }
                CDs *sd = cbin.celldesc(DSP()->CurMode());
                if (!sd) {
                    GEO()->setCurTx(txbak);
                    delete [] fname;
                    Log()->ErrorLog(mh::Internal,
                        "Internal error: can't open existing cell.");
                    return;
                }
                const BBox *BB = sd->BB();

                switch (vertJustify()) {
                case 1:
                    yos += BB->height()/2;
                    break;
                case 2:
                    yos += BB->height();
                    break;
                }
                switch (horzJustify()) {
                case 1:
                    xos += BB->width()/2;
                    break;
                case 2:
                    xos += BB->width();
                    break;
                }
            }
            else {
                GEO()->setCurTx(txbak);
                delete [] fname;
                Log()->ErrorLog(mh::ObjectCreation, "XPM file not found.");
                return;
            }
        }
        else if (ltype == LogoPretty) {
            int lwid, lhei, numlines;
            label::manh_text_extent(str, &lwid, &lhei, &numlines, true);
            switch (vertJustify()) {
            case 1:
                yos += (csdim*lhei)/2;
                break;
            case 2:
                yos += csdim*lhei;
                break;
            }
            switch (horzJustify()) {
            case 1:
                xos += (csdim*lwid)/2;
                break;
            case 2:
                xos += csdim*lwid;
                break;
            }
        }
        else if (ltype == LogoManhattan) {
            int lwid, lhei, numlines;
            label::manh_text_extent(str, &lwid, &lhei, &numlines, false);
            switch (vertJustify()) {
            case 1:
                yos += (csdim*lhei)/2;
                break;
            case 2:
                yos += csdim*lhei;
                break;
            }
            switch (horzJustify()) {
            case 1:
                xos += (csdim*lwid)/2;
                break;
            case 2:
                xos += csdim*lwid;
                break;
            }
        }
        else {
            int lwid, lhei, numlines;
            switch (vertJustify()) {
            case 1:
                LogoFT.textExtent(str, &lwid, &lhei, &numlines);
                yos += (csdim*lhei)/2;
                break;
            case 2:
                LogoFT.textExtent(str, &lwid, &lhei, &numlines);
                yos += csdim*lhei;
                break;
            }
            switch (horzJustify()) {
            case 1:
                xos += (LogoFT.lineExtent(str)*csdim)/2;
                break;
            case 2:
                xos += LogoFT.lineExtent(str)*csdim;
                break;
            }
        }

        CDs *cursd = CurCell();
        if (cursd)
            makeInstance(cursd, fname, x, y, xos, yos);
        GEO()->setCurTx(txbak);
        delete [] fname;
    }
}
// End of cEdit functions.


LabelState::LabelState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    Text = 0;
    Pushed = false;
    DoLogo = false;
    LogoMode = LogoNormal;
    MinPix = 1;
    Plist = 0;
    Xos = 0;
    Yos = 0;
    Pwidth = 0;
    Style = (WireStyle)DEF_LOGO_END_STYLE;
    PathWidth = DEF_LOGO_PATH_WIDTH;
    KeepLT = false;
    TxSaved = false;
    TxHJ = 0;
    TxVJ = 0;
    Target = 0;
    TrgWire = 0;
}


LabelState::~LabelState()
{
    LabelCmd = 0;
    PolyList::destroy(Plist);
    if (DoLogo)
        ED()->PopUpLogo(0, MODE_OFF);
}


void
LabelState::set_curtx(bool set)
{
    if (set) {
        if (!Target)
            return;
        TxBak = *GEO()->curTx();
        TxSaved = true;
        sCurTx ct;

        TxHJ = ED()->horzJustify();
        ED()->setHorzJustify(0);
        TxVJ = ED()->vertJustify();
        ED()->setVertJustify(0);

        int xform = Target->xform();

        if (xform & TXTF_HJR)
            ED()->setHorzJustify(2);
        else if (xform & TXTF_HJC)
            ED()->setHorzJustify(1);
        if (xform & TXTF_VJT)
            ED()->setVertJustify(2);
        else if (xform & TXTF_VJC)
            ED()->setVertJustify(1);

        if (xform & TXTF_MX)
            ct.set_reflectX(true);
        if (xform & TXTF_MY)
            ct.set_reflectY(true);

        switch (xform & 0x3) {
        case 0:
            if (xform & TXTF_45)
                ct.set_angle(45);
            break;
        case 1:
            if (xform & TXTF_45)
                ct.set_angle(135);
            else
                ct.set_angle(90);
            break;
        case 2:
            if (xform & TXTF_45)
                ct.set_angle(225);
            else
                ct.set_angle(180);
            break;
        case 3:
            if (xform & TXTF_45)
                ct.set_angle(315);
            else
                ct.set_angle(270);
            break;
        }
        GEO()->setCurTx(ct);
    }
    else {
        if (TxSaved)
            GEO()->setCurTx(TxBak);
    }
    XM()->ShowParameters();
}


void
LabelState::set_size(double mw, double mh)
{
    DisplayMode mode = DSP()->CurMode();
    double xw = (mode == Physical ? CDphysDefTextWidth : CDelecDefTextWidth);
    xw *= 5000;
    if (mw > xw) {
        double r = mh/mw;
        mw = xw;
        mh = mw*r;
    }
    double xh = (mode == Physical ? CDphysDefTextHeight : CDelecDefTextHeight);
    xh *= 5000;
    if (mh > xh) {
        double r = mw/mh;
        mh = xh;
        mw = mh*r;
    }
    GRvecFont *ft = DoLogo ? &LogoFT : &FT;
    if (mode == Physical) {
        xw = MICRONS(MinPix*ft->cellWidth());
        xh = MICRONS(MinPix*ft->cellHeight());
    }
    else {
        xw = ELEC_MICRONS(MinPix*ft->cellWidth());
        xh = ELEC_MICRONS(MinPix*ft->cellHeight());
    }
    if (mw < xw) {
        double r = mh/mw;
        mw = xw;
        mh = mw*r;
    }
    if (mh < xh) {
        double r = mw/mh;
        mh = xh;
        mw = mh*r;
    }
    if (mode == Physical) {
        DSP()->SetPhysCharWidth(mw);
        DSP()->SetPhysCharHeight(mh);
    }
    else {
        DSP()->SetElecCharWidth(mw);
        DSP()->SetElecCharHeight(mh);
    }
}


void
LabelState::esc()
{
    if (LabelCmd) {
        Gst()->SetGhost(GFnone);
        set_curtx(false);
        hyList::destroy(Text);
        if (!TrgWire)
            PL()->ErasePrompt();
        if (Pushed)
            EV()->PopCallback(this);
        Menu()->Deselect(Caller);
        if (!KeepLT)
            // pop down long text editor
            PL()->AbortLongText();
        delete this;
    }
}


void
LabelState::b1down()
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    if (!DoLogo) {
        if (Target) {
            int x, y;
            EV()->Cursor().get_xy(&x, &y);
            int w, h;
            DSP()->LabelSize(Text, DSP()->CurMode(), &w, &h);
            ED()->setLabelOverride(x, y, w, h, current_xform());
            CDla *tl = ED()->changeLabel(Target, cursd, Text);
            ED()->setLabelOverride(0, 0, 0, 0, 0);
            if (tl)
                Selections.removeObject(cursd, tl);
            Ulist()->CommitChanges(true);
            esc();
            if (DSP()->CurMode() == Electrical)
                ScedIf()->PopUpNodeMap(0, MODE_UPD);
            XM()->ShowParameters();
            return;
        }
        // make a label
        CDl *ld = LT()->CurLayer();
        Gst()->SetGhost(GFnone);
        Label label;
        EV()->Cursor().get_xy(&label.x, &label.y);
        label.label = Text;
        label.xform = current_xform();
        DSP()->LabelSize(Text, DSP()->CurMode(), &label.width, &label.height);
        Errs()->init_error();
        CDla *newla = cursd->newLabel(0, &label, ld, 0, true);
        if (!newla) {
            Errs()->add_error("newLabel failed");
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            return;
        }
        if (TrgWire) {
            // Link it to the wire.
            if (!TrgWire->set_node_label(cursd, newla)) {
                PL()->ShowPrompt("Error linking label to selected wire.");
                Errs()->add_error("Failed to link label to selected wire.");
                Log()->ErrorLog(mh::Internal, Errs()->get_error());
            }
            else {
                PL()->ShowPrompt("New label linked to selected wire, "
                    "this will set node name.");
            }
        }
        DSP()->RedisplayArea(&newla->oBB());
        Ulist()->CommitChanges();
        if (TrgWire) {
            ScedIf()->PopUpNodeMap(0, MODE_UPD);
            esc();
        }
        else
            Gst()->SetGhost(GFlabel);
    }
    else {
        Ulist()->ListCheck(StateName, cursd, false);
        Gst()->SetGhost(GFnone);
        char *str = hyList::string(Text, HYcvPlain, false);
        int x, y;
        EV()->Cursor().get_xy(&x, &y);
        ED()->createLogo(str, x, y,
            quantize(INTERNAL_UNITS(DSP()->PhysCharWidth()/
            LogoFT.cellWidth())));

        delete [] str;
        // Turn off interactive DRC.
        Ulist()->CommitChanges(true, true);
        Gst()->SetGhost(GFlabel);
    }
}


namespace {
    // Set the LogoPathWidth variable.
    //
    void
    setpw(int pw)
    {
        if (pw != DEF_LOGO_PATH_WIDTH) {
            char buf[4];
            buf[1] = 0;
            buf[0] = '0' + pw;
            CDvdb()->setVariable(VA_LogoPathWidth, buf);
        }
        else
            CDvdb()->clearVariable(VA_LogoPathWidth);
    }
}


bool
LabelState::key(int code, const char*, int mstate)
{
    switch (code) {
    case RETURN_KEY:
        if (Target) {
            int w, h;
            DSP()->LabelSize(Text, DSP()->CurMode(), &w, &h);
            ED()->setLabelOverride(Target->xpos(), Target->ypos(), w, h,
                current_xform());
            CDla *tl = ED()->changeLabel(Target, CurCell(), Text);
            ED()->setLabelOverride(0, 0, 0, 0, 0);
            if (tl)
                Selections.removeObject(CurCell(), tl);
            Ulist()->CommitChanges(true);
            esc();
            if (DSP()->CurMode() == Electrical)
                ScedIf()->PopUpNodeMap(0, MODE_UPD);
            XM()->ShowParameters();
        }
        return (true);

    case LEFT_KEY:
        if (mstate & GR_CONTROL_MASK) {
            if ((mstate & GR_SHIFT_MASK) && DoLogo) {
                Gst()->SetGhost(GFnone);
                if (PathWidth > 1)
                    PathWidth--;
                setpw(PathWidth);
                Gst()->SetGhost(GFlabel);
                return (true);
            }
            break;
        }
        if (mstate & GR_SHIFT_MASK) {
            Gst()->SetGhost(GFnone);
            ED()->decHorzJustify();
            PolyList::destroy(Plist);
            Plist = 0;
            Gst()->SetGhost(GFlabel);
            return (true);
        }
        if (DSP()->CurMode() == Physical && LabelCmd->DoLogo) {
            // If overriding size, ignore arrow button size change.
            if (LogoPixelSize)
                break;
        }
        Gst()->SetGhost(GFnone);
        if (DSP()->CurMode() == Physical) {
            set_size(DSP()->PhysCharWidth()/FIFTH_ROOT_TWO,
                DSP()->PhysCharHeight()/FIFTH_ROOT_TWO);
        }
        else {
            set_size(DSP()->ElecCharWidth()/FIFTH_ROOT_TWO,
                DSP()->ElecCharHeight()/FIFTH_ROOT_TWO);
        }
        Gst()->SetGhost(GFlabel);
        return (true);

    case DOWN_KEY:
        if (mstate & GR_CONTROL_MASK) {
            if ((mstate & GR_SHIFT_MASK) && DoLogo) {
                Gst()->SetGhost(GFnone);
                if (PathWidth > 1)
                    PathWidth--;
                setpw(PathWidth);
                Gst()->SetGhost(GFlabel);
                return (true);
            }
            break;
        }
        if (mstate & GR_SHIFT_MASK) {
            Gst()->SetGhost(GFnone);
            ED()->decVertJustify();
            PolyList::destroy(Plist);
            Plist = 0;
            Gst()->SetGhost(GFlabel);
            return (true);
        }
        if (DSP()->CurMode() == Physical && LabelCmd->DoLogo) {
            // If overriding size, ignore arrow button size change.
            if (LogoPixelSize)
                break;
        }
        Gst()->SetGhost(GFnone);
        if (DSP()->CurMode() == Physical)
            set_size(0.5*DSP()->PhysCharWidth(), 0.5*DSP()->PhysCharHeight());
        else
            set_size(0.5*DSP()->ElecCharWidth(), 0.5*DSP()->ElecCharHeight());
        Gst()->SetGhost(GFlabel);
        return (true);

    case RIGHT_KEY:
        if (mstate & GR_CONTROL_MASK) {
            if ((mstate & GR_SHIFT_MASK) && DoLogo) {
                Gst()->SetGhost(GFnone);
                if (PathWidth < 5)
                    PathWidth++;
                setpw(PathWidth);
                Gst()->SetGhost(GFlabel);
                return (true);
            }
            break;
        }
        if (mstate & GR_SHIFT_MASK) {
            Gst()->SetGhost(GFnone);
            ED()->incHorzJustify();
            PolyList::destroy(Plist);
            Plist = 0;
            Gst()->SetGhost(GFlabel);
            return (true);
        }
        if (DSP()->CurMode() == Physical && LabelCmd->DoLogo) {
            // If overriding size, ignore arrow button size change.
            if (LogoPixelSize)
                break;
        }
        Gst()->SetGhost(GFnone);
        if (DSP()->CurMode() == Physical) {
            set_size(FIFTH_ROOT_TWO*DSP()->PhysCharWidth(),
                FIFTH_ROOT_TWO*DSP()->PhysCharHeight());
        }
        else {
            set_size(FIFTH_ROOT_TWO*DSP()->ElecCharWidth(),
                FIFTH_ROOT_TWO*DSP()->ElecCharHeight());
        }
        Gst()->SetGhost(GFlabel);
        return (true);

    case UP_KEY:
        if (mstate & GR_CONTROL_MASK) {
            if ((mstate & GR_SHIFT_MASK) && DoLogo) {
                Gst()->SetGhost(GFnone);
                if (PathWidth < 5)
                    PathWidth++;
                setpw(PathWidth);
                Gst()->SetGhost(GFlabel);
                return (true);
            }
            break;
        }
        if (mstate & GR_SHIFT_MASK) {
            Gst()->SetGhost(GFnone);
            ED()->incVertJustify();
            PolyList::destroy(Plist);
            Plist = 0;
            Gst()->SetGhost(GFlabel);
            return (true);
        }
        if (DSP()->CurMode() == Physical && LabelCmd->DoLogo) {
            // If overriding size, ignore arrow button size change.
            if (LogoPixelSize)
                break;
        }
        Gst()->SetGhost(GFnone);
        if (DSP()->CurMode() == Physical)
            set_size(2.0*DSP()->PhysCharWidth(), 2.0*DSP()->PhysCharHeight());
        else
            set_size(2.0*DSP()->ElecCharWidth(), 2.0*DSP()->ElecCharHeight());
        Gst()->SetGhost(GFlabel);
        return (true);

    case DELETE_KEY:
        DSPmainDraw(ShowGhost(ERASE))
        bool waslogo = LabelCmd->DoLogo;
        hyList *hlabel = PL()->EditHypertextPrompt("Enter new label: ", Text,
            false);
        if (!hlabel || (hlabel->ref_type() == HLrefText &&
                !hlabel->text()[0])) {
            if (hlabel)
                hyList::destroy(hlabel);
            if (LabelCmd && LabelCmd->DoLogo == waslogo)
                esc();
            return (true);
        }
        hyList::destroy(Text);
        Text = hlabel;
        PolyList::destroy(Plist);
        Plist = 0;
        DSPmainDraw(ShowGhost(DISPLAY))
        ED()->logoUpdate();
        return (true);
    }
    return (false);
}


int
LabelState::current_xform()
{
    int xform = 0;
    if (Target) {
        xform = Target->xform();
        xform &= ~(TXTF_XF | TXTF_HJC | TXTF_HJR | TXTF_VJC | TXTF_VJT);
    }

    int zz = GEO()->curTx()->angle()/45;
    xform |= (zz >> 1) & 0x3;
    if (zz & 1)
        xform |= TXTF_45;
    if (GEO()->curTx()->reflectY())
        xform |= TXTF_MY;
    if (GEO()->curTx()->reflectX())
        xform |= TXTF_MX;
    xform &= ~(TXTF_HJC | TXTF_HJR | TXTF_VJC | TXTF_VJT);
    if (ED()->horzJustify() == 1)
        xform |= TXTF_HJC;
    else if (ED()->horzJustify() == 2)
        xform |= TXTF_HJR;
    if (ED()->vertJustify() == 1)
        xform |= TXTF_VJC;
    else if (ED()->vertJustify() == 2)
        xform |= TXTF_VJT;
    return (xform);
}
// End of LabelState methods


namespace {
    // This returns the x offset of the start of the line for logo functions
    //
    int
    xoffset(const char *string, int pix_width)
    {
        int xos = pix_width;
        switch (ED()->horzJustify()) {
        case 1:
            xos -= (LogoFT.lineExtent(string)*pix_width)/2;
            break;
        case 2:
            xos -= LogoFT.lineExtent(string)*pix_width;
            break;
        }
        return (xos);
    }
}


// Create the physical label.
//
void
label::write_logo(const cTfmStack *tstk, CDl *ld, const char *string,
    int width, int height, int pwidth, WireStyle ws)
{
    CDs *cursd = CurCell();
    if (!cursd)
        return;
    if (is_xpm(string)) {
        int lwid, lhei;
        if (!xpm_size(string, &lwid, &lhei)) {
            Log()->PopUpErr(
                "XPM file not found or not recognized, press Delete\n"
                "to reenter.");
            return;
        }
        int yos = 0;
        switch (ED()->vertJustify()) {
        case 1:
            yos -= (height*lhei)/2;
            break;
        case 2:
            yos -= height*lhei;
            break;
        }
        int xos = width;
        switch (ED()->horzJustify()) {
        case 1:
            xos -= (width*lwid)/2;
            break;
        case 2:
            xos -= width*lwid;
            break;
        }

        Zlist *z = xpm_to_zlist(string, width, xos, yos);
        PolyList *plist = Zlist::to_poly_list(z);
        for (PolyList *p = plist; p; p = p->next) {
            tstk->TPath(p->po.numpts, p->po.points);
            cursd->newPoly(0, &p->po, ld, 0, false);
        }
        PolyList::destroy(plist);
    }
    else {
        int dd;
        if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoAltFont)) &&
                dd >= 0 && dd <= 1) {
            bool pretty = (dd == 1);
            int lwid, lhei, numlines;
            manh_text_extent(string, &lwid, &lhei, &numlines, pretty);
            int yos = 0;
            if (numlines > 1)
                yos = (lhei - lhei/numlines)*width;

            switch (ED()->vertJustify()) {
            case 1:
                yos -= (width*lhei)/2;
                break;
            case 2:
                yos -= width*lhei;
                break;
            }
            int xos = width;
            switch (ED()->horzJustify()) {
            case 1:
                xos -= (width*lwid)/2;
                break;
            case 2:
                xos -= width*lwid;
                break;
            }

            PolyList *plist =
                string_to_polys(string, width, xos, yos, lwid, pretty);
            for (PolyList *p = plist; p; p = p->next) {
                tstk->TPath(p->po.numpts, p->po.points);
                cursd->newPoly(0, &p->po, ld, 0, false);
            }
            PolyList::destroy(plist);
        }
        else {
            int lwid, lhei, numlines;
            LogoFT.textExtent(string, &lwid, &lhei, &numlines);
            // compute offsets for justification
            int yos = numlines > 1 ?
                (numlines-1)*height*LogoFT.cellHeight() : 0;
            switch (ED()->vertJustify()) {
            case 1:
                yos -= (height*lhei)/2;
                break;
            case 2:
                yos -= height*lhei;
                break;
            }
            int xos = xoffset(string, width);

            for ( ; *string; string++) {
                GRvecFont::Character *cp = LogoFT.entry(*string);
                if (cp) {
                    for (int i = 0; i < cp->numstroke; i++) {
                        GRvecFont::Cstroke *stroke = &cp->stroke[i];
                        Point *pts = new Point[stroke->numpts];
                        for (int j = 0; j < stroke->numpts; j++) {
                            pts[j].set(
                                width*(stroke->cp[j].x - cp->ofst) + xos,
                                height*(LogoFT.cellHeight() - 1 -
                                stroke->cp[j].y) + yos);
                            tstk->TPoint(&pts[j].x, &pts[j].y);
                        }
                        Wire wire(pwidth, ws, stroke->numpts, pts);
                        cursd->newWire(0, &wire, ld, 0, false);
                    }
                    xos += width*cp->width;
                }
                else if (*string == '\n') {
                    xos = xoffset(string+1, width);
                    yos -= height*LogoFT.cellHeight();
                }
                else
                    xos += width*LogoFT.cellWidth();
            }
        }
    }
}


// Write a file containing the physical label.
//
void
label::write_logofile(FILE *fp, const char *name, const char *string,
    int width, int height, int pwidth, WireStyle ws, const char *docstr)
{
    fprintf(fp, "(Symbol %s);\n", name);
    if (CDphysResolution != 100)
        fprintf(fp, "(RESOLUTION %d);\n", CDphysResolution);
    if (docstr && *docstr)
        Gen.Comment(fp, docstr);
    fprintf(fp, "9 %s;\n", name);
    Gen.BeginSymbol(fp, 0, 1, 1);
    Gen.Layer(fp, LT()->CurLayer()->name());

    if (is_xpm(string)) {
        int yos = 0;
        int xos = width;

        Zlist *z = xpm_to_zlist(string, width, xos, yos);
        PolyList *plist = Zlist::to_poly_list(z);
        for (PolyList *p = plist; p; p = p->next)
            Gen.Polygon(fp, p->po.points, p->po.numpts);
        PolyList::destroy(plist);
    }
    else {
        int dd;
        if (str_to_int(&dd, CDvdb()->getVariable(VA_LogoAltFont)) &&
                dd >= 0 && dd <= 1) {
            bool pretty = (dd == 1);
            int lwid, lhei, numlines;
            manh_text_extent(string, &lwid, &lhei, &numlines, pretty);
            int xos = width;
            int yos = 0;
            if (numlines > 1)
                yos = (lhei - lhei/numlines)*width;

            PolyList *plist =
                string_to_polys(string, width, xos, yos, lwid, pretty);
            for (PolyList *p = plist; p; p = p->next)
                Gen.Polygon(fp, p->po.points, p->po.numpts);
            PolyList::destroy(plist);
        }
        else {
            int lwid, lhei, numlines;
            LogoFT.textExtent(string, &lwid, &lhei, &numlines);
            int yos = numlines > 1 ?
                (numlines-1)*height*LogoFT.cellHeight() : 0;
            int xos = width;

            for ( ; *string; string++) {
                GRvecFont::Character *cp = LogoFT.entry(*string);
                if (cp) {
                    for (int i = 0; i < cp->numstroke; i++) {
                        GRvecFont::Cstroke *stroke = &cp->stroke[i];
                        Point *pts = new Point[stroke->numpts];
                        for (int j = 0; j < stroke->numpts; j++) {
                            pts[j].set(
                                width*(stroke->cp[j].x - cp->ofst) + xos,
                                height*
                                    (LogoFT.cellHeight() - 1 -
                                    stroke->cp[j].y) + yos);
                        }
                        Gen.Wire(fp, pwidth, ws, pts, stroke->numpts);
                        delete [] pts;
                    }
                    xos += width*cp->width;
                }
                else if (*string == '\n') {
                    xos = width;
                    yos -= height*LogoFT.cellHeight();
                }
                else
                    xos += width*LogoFT.cellWidth();
            }
        }
    }
    Gen.EndSymbol(fp);
    Gen.End(fp);
}


namespace {
    // Implement a modified `base64' encoding.  This becomes part of the
    // file name, so we use GDSII friendly ascii characters only.
    //
    const char base64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789$&";

    char *
    to_base64(const unsigned char *buf, size_t len)
    {
        char *s = new char [(4*(len + 1))/3 + 1];
        char *rv = s;
        unsigned tmp;
        while (len >= 3) {
            tmp = buf[0] << 16 | buf[1] << 8 | buf[2];
            s[0] = base64[tmp >> 18];
            s[1] = base64[(tmp >> 12) & 077];
            s[2] = base64[(tmp >> 6) & 077];
            s[3] = base64[tmp & 077];
            len -= 3;
            buf += 3;
            s += 4;
        }

        // RFC 1521 enumerates these three possibilities...
        switch(len) {
        case 2:
            tmp = buf[0] << 16 | buf[1] << 8;
            s[0] = base64[(tmp >> 18) & 077];
            s[1] = base64[(tmp >> 12) & 077];
            s[2] = base64[(tmp >> 6) & 077];
            s[3] = '\0';
            s[4] = '\0';
            break;
        case 1:
            tmp = buf[0] << 16;
            s[0] = base64[(tmp >> 18) & 077];
            s[1] = base64[(tmp >> 12) & 077];
            s[2] = s[3] = '\0';
            s[4] = '\0';
            break;
        case 0:
            s[0] = '\0';
            break;
        }

        return (rv);
    }
}


// Create a file name for the logo string.  Arguments:
//  string       the logo text or xpm file name
//  lname        layer name
//  cswid        charcell width
//  pw           path width param (1-5, or 0 for Manhattan font)
//  hj           horiz justification (0-2)
//  st           end style (0-2, 0 for Manhattan font)
//
char *
label::new_filename(const char *string, const char *lname, int csdim,
    int pw, int hj, int st)
{
    char *docstr = new char[strlen(string) + 64];
    sprintf(docstr,
        "string=\"%s\" layer=%s dim=%d pw=%d hj=%d style=%d",
        string, lname, csdim, pw, hj, st);
    MD5cx ctx;
    ctx.update((unsigned char*)docstr, strlen(docstr));
    unsigned char final[16];
    ctx.final(final);
    char *es = to_base64(final, 16);

    char strbuf[12];
    strncpy(strbuf, string, 8);
    strbuf[8] = '\0';
    char *s = strbuf;
    while (*s) {
        if (!isalpha(*s) && !isdigit(*s))
            *s = *s%26 + 'a';
        s++;
    }
    char buf[64];
    sprintf(buf, "%s%s", strbuf, es);
    delete [] es;

    return (lstring::copy(buf));
}


namespace {Zlist *char_to_zlist(int, int, int, int); }

// Return a list of Manhattan polygons representing the character string.
// Argument d is the "pixel" size.  The lower left corner of the top line
// is at x, y.
//
PolyList *
label::string_to_polys(const char *string, int d, int x, int y, int lwid,
    bool dopretty)
{
    if (!string)
        return (0);
    if (dopretty)
        return (cEdit::polytext(string, d, x, y));

    PolyList *p0 = 0, *pe = 0;
    int x0 = x;
    for (const char *s = string; *s; s++) {
        if (*s == '\n') {
            const char *t = strchr(s+1, '\n');
            int len = t ? t - s - 1 : strlen(s+1);
            x = x0;
            switch (ED()->horzJustify()) {
            case 1:
                x += d*(lwid - len*8)/2;
                break;
            case 2:
                x += d*(lwid - len*8);
                break;
            }
            y -= 16*d;
            continue;
        }
        Zlist *z = char_to_zlist(*s, d, x, y);
        if (z) {
            if (!p0)
                p0 = pe = Zlist::to_poly_list(z);
            else
                pe->next = Zlist::to_poly_list(z);
            while (pe->next)
                pe = pe->next;
        }
        x += d*8;
    }
    return (p0);
}


// Text extent function for the Manhattan font.
//
void
label::manh_text_extent(const char *text, int *width, int *height,
    int *numlines, bool dopretty)
{
    if (dopretty) {
        cEdit::polytextExtent(text, width, height, numlines);
        return;
    }
    int mlen = 0;
    int nl = 1;
    const char *t = text;
    const char *s = strchr(t, '\n');
    while (s) {
        nl++;
        int len = s - t;
        if (mlen < len)
            mlen = len;
        t = s+1;
        s = strchr(t, '\n');
    }
    int len = strlen(t);
    if (mlen < len)
        mlen = len;
    t = text + strlen(text) - 1;
    if (*t == '\n')
        nl--;
    *width = mlen*8;
    *height = nl*16;
    *numlines = nl;
}


//
// Functions for reading and processing XPM images.
//

// If string is a single token ending with ".xpm", return true.
//
bool
label::is_xpm(const char *string)
{
    if (!string)
        return (false);
    const char *t = strrchr(string, '.');
    if (!t)
        return (false);
    if (!lstring::cieq(t+1, "xpm"))
        return (false);
    for ( ; t >= string; t--) {
        if (isspace(*t))
            return (false);
    }
    return (true);
}


// Read the given xpm file, and convert non-background pixels to a zoid
// list.  The zoids are square with size d, and are sorted t->b, l->r.
// The lower left is at x, y.
//
Zlist *
label::xpm_to_zlist(const char *fname, int d, int x, int y)
{
    FILE *fp = fopen(fname, "r");
    if (!fp)
         return (0);
    char **xpm = 0;
    int nlines = 0;
    int cnt = 0;
    // save all quoted lines in order, removing quotes
    for (;;) {
        int qcnt = 0;
        int c;
        sLstr lstr;
        while ((c = getc(fp)) != EOF) {
            if (c == '\r')
                continue;
            if (c == '\n')
                break;
            if (c == '"') {
                qcnt++;
                continue;
            }
            if (qcnt == 2)
                continue;
            if (isspace(c) && !qcnt)
                continue;
            lstr.add_c(c);
        }
        if (c == EOF)
            break;
        if (qcnt) {
            char *line = lstr.string_trim();
            if (!line)
                break;
            if (cnt >= nlines) {
                nlines += 10;
                char **tmp = new char*[nlines];
                if (xpm) {
                    memcpy(tmp, xpm, cnt*sizeof(char*));
                    delete [] xpm;
                }
                xpm = tmp;
                for (int i = cnt+1; i < nlines; i++)
                    xpm[i] = 0;
            }
            xpm[cnt] = line;
            cnt++;
        }
    }
    fclose(fp);
    int lcnt = cnt;

    int nx, ny, nc, xx;
    if (sscanf(xpm[0], "%d %d %d %d", &nx, &ny, &nc, &xx) != 4) {
        // bummer
        for (int i = 0; i < lcnt; i++) {
            delete [] xpm[i];
            xpm[i] = 0;
        }
        delete [] xpm;
        return (0);
    }
    int bg = *xpm[1];  // background pixel char
    cnt = nc + 1;  // start of map lines

    Zlist *zl0 = 0, *ze = 0;
    int row = lcnt - cnt - 1;
    for ( ; cnt < lcnt; cnt++, row--) {
        int col = 0;
        for (char *s = xpm[cnt]; *s; s++, col++) {
            if (*s == bg)
                continue;
            Zoid Z(x + col*d, x + (col+1)*d, y + row*d,
                x + col*d, x + (col+1)*d, y + (row+1)*d);
            if (!zl0)
                zl0 = ze = new Zlist(&Z);
            else {
                ze->next = new Zlist(&Z);
                ze = ze->next;
            }
        }
    }
    for (int i = 0; i < lcnt; i++) {
        delete [] xpm[i];
        xpm[i] = 0;
    }
    delete [] xpm;
    return (zl0);
}


// Open the xpm file and grab the map size.  If success, return true.
//
bool
label::xpm_size(const char *fname, int *x, int *y)
{
    // Used to cache x,y and the file name for matching, not a good
    // idea since users may change the content while keeping the same
    // file name.

    FILE *fp = fopen(fname, "r");
    if (fp) {
        char buf[256];
        int cnt = 0;
        while (fgets(buf, 256, fp) != 0) {
            char *s = buf;
            while (isspace(*s))
                s++;
            if (*s == '"') {
                fclose(fp);
                int nc, xx;
                if (sscanf(s+1, "%d %d %d %d", x, y, &nc, &xx) != 4)
                    return (false);
                return (true);
            }
            if (cnt++ > 5)
                 break;
        }
        fclose(fp);
    }
    return (false);
}


//-----------------------------------------------------------------------------
// An alternative logo font that will render characters as Manhattan
// polygons.
//

namespace {
    const unsigned char an8X16font[] = {
    // !
    0x0,0x0,0x18,0x3c,0x3c,0x3c,0x18,0x18,0x18,0x0,0x18,0x18,0x0,0x0,0x0,0x0,
    // "
    0x0,0x66,0x66,0x66,0x24,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    // #
    0x0,0x0,0x0,0x6c,0x6c,0xfe,0x6c,0x6c,0x6c,0xfe,0x6c,0x6c,0x0,0x0,0x0,0x0,
    // $
    0x18,0x18,0x7c,0xc6,0xc2,0xc0,0x7c,0x6,0x6,0x86,0xc6,0x7c,0x18,0x18,0x0,0x0,
    // %
    0x0,0x0,0x0,0x0,0xc2,0xc6,0xc,0x18,0x30,0x60,0xc6,0x86,0x0,0x0,0x0,0x0,
    // &
    0x0,0x0,0x38,0x6c,0x6c,0x38,0x76,0xdc,0xcc,0xcc,0xcc,0x76,0x0,0x0,0x0,0x0,
    // '
    0x0,0x30,0x30,0x30,0x60,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    // (
    0x0,0x0,0xc,0x18,0x30,0x30,0x30,0x30,0x30,0x30,0x18,0xc,0x0,0x0,0x0,0x0,
    // )
    0x0,0x0,0x30,0x18,0xc,0xc,0xc,0xc,0xc,0xc,0x18,0x30,0x0,0x0,0x0,0x0,
    // *
    0x0,0x0,0x0,0x0,0x0,0x66,0x3c,0xff,0x3c,0x66,0x0,0x0,0x0,0x0,0x0,0x0,
    // +
    0x0,0x0,0x0,0x0,0x0,0x18,0x18,0x7e,0x18,0x18,0x0,0x0,0x0,0x0,0x0,0x0,
    // ,
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0x18,0x18,0x30,0x0,0x0,0x0,
    // -
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x7e,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    // .
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18,0x18,0x0,0x0,0x0,0x0,
    // /
    0x0,0x0,0x0,0x0,0x2,0x6,0xc,0x18,0x30,0x60,0xc0,0x80,0x0,0x0,0x0,0x0,
    // 0
    0x0,0x0,0x7c,0xc6,0xc6,0xce,0xde,0xf6,0xe6,0xc6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // 1
    0x0,0x0,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x7e,0x0,0x0,0x0,0x0,
    // 2
    0x0,0x0,0x7c,0xc6,0x6,0xc,0x18,0x30,0x60,0xc0,0xc6,0xfe,0x0,0x0,0x0,0x0,
    // 3
    0x0,0x0,0x7c,0xc6,0x6,0x6,0x3c,0x6,0x6,0x6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // 4
    0x0,0x0,0xc,0x1c,0x3c,0x6c,0xcc,0xfe,0xc,0xc,0xc,0x1e,0x0,0x0,0x0,0x0,
    // 5
    0x0,0x0,0xfe,0xc0,0xc0,0xc0,0xfc,0x6,0x6,0x6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // 6
    0x0,0x0,0x38,0x60,0xc0,0xc0,0xfc,0xc6,0xc6,0xc6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // 7
    0x0,0x0,0xfe,0xc6,0x6,0x6,0xc,0x18,0x30,0x30,0x30,0x30,0x0,0x0,0x0,0x0,
    // 8
    0x0,0x0,0x7c,0xc6,0xc6,0xc6,0x7c,0xc6,0xc6,0xc6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // 9
    0x0,0x0,0x7c,0xc6,0xc6,0xc6,0x7e,0x6,0x6,0x6,0xc,0x78,0x0,0x0,0x0,0x0,
    // :
    0x0,0x0,0x0,0x0,0x18,0x18,0x0,0x0,0x0,0x18,0x18,0x0,0x0,0x0,0x0,0x0,
    // ;
    0x0,0x0,0x0,0x0,0x18,0x18,0x0,0x0,0x0,0x18,0x18,0x30,0x0,0x0,0x0,0x0,
    // <
    0x0,0x0,0x0,0x6,0xc,0x18,0x30,0x60,0x30,0x18,0xc,0x6,0x0,0x0,0x0,0x0,
    // =
    0x0,0x0,0x0,0x0,0x0,0x7e,0x0,0x0,0x7e,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    // >
    0x0,0x0,0x0,0x60,0x30,0x18,0xc,0x6,0xc,0x18,0x30,0x60,0x0,0x0,0x0,0x0,
    // ?
    0x0,0x0,0x7c,0xc6,0xc6,0xc,0x18,0x18,0x18,0x0,0x18,0x18,0x0,0x0,0x0,0x0,
    // @
    0x0,0x0,0x7c,0xc6,0xc6,0xc6,0xde,0xde,0xde,0xdc,0xc0,0x7c,0x0,0x0,0x0,0x0,
    // A
    0x0,0x0,0x10,0x38,0x6c,0xc6,0xc6,0xfe,0xc6,0xc6,0xc6,0xc6,0x0,0x0,0x0,0x0,
    // B
    0x0,0x0,0xfc,0x66,0x66,0x66,0x7c,0x66,0x66,0x66,0x66,0xfc,0x0,0x0,0x0,0x0,
    // C
    0x0,0x0,0x3c,0x66,0xc2,0xc0,0xc0,0xc0,0xc0,0xc2,0x66,0x3c,0x0,0x0,0x0,0x0,
    // D
    0x0,0x0,0xf8,0x6c,0x66,0x66,0x66,0x66,0x66,0x66,0x6c,0xf8,0x0,0x0,0x0,0x0,
    // E
    0x0,0x0,0xfe,0x66,0x62,0x68,0x78,0x68,0x60,0x62,0x66,0xfe,0x0,0x0,0x0,0x0,
    // F
    0x0,0x0,0xfe,0x66,0x62,0x68,0x78,0x68,0x60,0x60,0x60,0xf0,0x0,0x0,0x0,0x0,
    // G
    0x0,0x0,0x3c,0x66,0xc2,0xc0,0xc0,0xde,0xc6,0xc6,0x66,0x3a,0x0,0x0,0x0,0x0,
    // H
    0x0,0x0,0xc6,0xc6,0xc6,0xc6,0xfe,0xc6,0xc6,0xc6,0xc6,0xc6,0x0,0x0,0x0,0x0,
    // I
    0x0,0x0,0x3c,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x0,0x0,0x0,0x0,
    // J
    0x0,0x0,0x1e,0xc,0xc,0xc,0xc,0xc,0xcc,0xcc,0xcc,0x78,0x0,0x0,0x0,0x0,
    // K
    0x0,0x0,0xe6,0x66,0x66,0x6c,0x78,0x78,0x6c,0x66,0x66,0xe6,0x0,0x0,0x0,0x0,
    // L
    0x0,0x0,0xf0,0x60,0x60,0x60,0x60,0x60,0x60,0x62,0x66,0xfe,0x0,0x0,0x0,0x0,
    // M
    0x0,0x0,0xc6,0xee,0xfe,0xfe,0xd6,0xc6,0xc6,0xc6,0xc6,0xc6,0x0,0x0,0x0,0x0,
    // N
    0x0,0x0,0xc6,0xe6,0xf6,0xfe,0xde,0xce,0xc6,0xc6,0xc6,0xc6,0x0,0x0,0x0,0x0,
    // O
    0x0,0x0,0x7c,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // P
    0x0,0x0,0xfc,0x66,0x66,0x66,0x7c,0x60,0x60,0x60,0x60,0xf0,0x0,0x0,0x0,0x0,
    // Q
    0x0,0x0,0x7c,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xd6,0xde,0x7c,0xc,0xe,0x0,0x0,
    // R
    0x0,0x0,0xfc,0x66,0x66,0x66,0x7c,0x6c,0x66,0x66,0x66,0xe6,0x0,0x0,0x0,0x0,
    // S
    0x0,0x0,0x7c,0xc6,0xc6,0x60,0x38,0xc,0x6,0xc6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // T
    0x0,0x0,0x7e,0x7e,0x5a,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x0,0x0,0x0,0x0,
    // U
    0x0,0x0,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // V
    0x0,0x0,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0x6c,0x38,0x10,0x0,0x0,0x0,0x0,
    // W
    0x0,0x0,0xc6,0xc6,0xc6,0xc6,0xd6,0xd6,0xd6,0xfe,0xee,0x6c,0x0,0x0,0x0,0x0,
    // X
    0x0,0x0,0xc6,0xc6,0x6c,0x7c,0x38,0x38,0x7c,0x6c,0xc6,0xc6,0x0,0x0,0x0,0x0,
    // Y
    0x0,0x0,0x66,0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x18,0x3c,0x0,0x0,0x0,0x0,
    // Z
    0x0,0x0,0xfe,0xc6,0x86,0xc,0x18,0x30,0x60,0xc2,0xc6,0xfe,0x0,0x0,0x0,0x0,
    // [
    0x0,0x0,0x3c,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3c,0x0,0x0,0x0,0x0,
    // backslash
    0x0,0x0,0x0,0x80,0xc0,0xe0,0x70,0x38,0x1c,0xe,0x6,0x2,0x0,0x0,0x0,0x0,
    // ]
    0x0,0x0,0x3c,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0xc,0x3c,0x0,0x0,0x0,0x0,
    // ^
    0x10,0x38,0x6c,0xc6,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    // _
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xff,0x0,0x0,
    // `
    0x30,0x30,0x18,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    // a
    0x0,0x0,0x0,0x0,0x0,0x78,0xc,0x7c,0xcc,0xcc,0xcc,0x76,0x0,0x0,0x0,0x0,
    // b
    0x0,0x0,0xe0,0x60,0x60,0x78,0x6c,0x66,0x66,0x66,0x66,0x7c,0x0,0x0,0x0,0x0,
    // c
    0x0,0x0,0x0,0x0,0x0,0x7c,0xc6,0xc0,0xc0,0xc0,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // d
    0x0,0x0,0x1c,0xc,0xc,0x3c,0x6c,0xcc,0xcc,0xcc,0xcc,0x76,0x0,0x0,0x0,0x0,
    // e
    0x0,0x0,0x0,0x0,0x0,0x7c,0xc6,0xfe,0xc0,0xc0,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // f
    0x0,0x0,0x38,0x6c,0x64,0x60,0xf0,0x60,0x60,0x60,0x60,0xf0,0x0,0x0,0x0,0x0,
    // g
    0x0,0x0,0x0,0x0,0x0,0x76,0xcc,0xcc,0xcc,0xcc,0xcc,0x7c,0xc,0xcc,0x78,0x0,
    // h
    0x0,0x0,0xe0,0x60,0x60,0x6c,0x76,0x66,0x66,0x66,0x66,0xe6,0x0,0x0,0x0,0x0,
    // i
    0x0,0x0,0x18,0x18,0x0,0x38,0x18,0x18,0x18,0x18,0x18,0x3c,0x0,0x0,0x0,0x0,
    // j
    0x0,0x0,0x6,0x6,0x0,0xe,0x6,0x6,0x6,0x6,0x6,0x6,0x66,0x66,0x3c,0x0,
    // k
    0x0,0x0,0xe0,0x60,0x60,0x66,0x6c,0x78,0x78,0x6c,0x66,0xe6,0x0,0x0,0x0,0x0,
    // l
    0x0,0x0,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3c,0x0,0x0,0x0,0x0,
    // m
    0x0,0x0,0x0,0x0,0x0,0xec,0xfe,0xd6,0xd6,0xd6,0xd6,0xc6,0x0,0x0,0x0,0x0,
    // n
    0x0,0x0,0x0,0x0,0x0,0xdc,0x66,0x66,0x66,0x66,0x66,0x66,0x0,0x0,0x0,0x0,
    // o
    0x0,0x0,0x0,0x0,0x0,0x7c,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // p
    0x0,0x0,0x0,0x0,0x0,0xdc,0x66,0x66,0x66,0x66,0x66,0x7c,0x60,0x60,0xf0,0x0,
    // q
    0x0,0x0,0x0,0x0,0x0,0x76,0xcc,0xcc,0xcc,0xcc,0xcc,0x7c,0xc,0xc,0x1e,0x0,
    // r
    0x0,0x0,0x0,0x0,0x0,0xdc,0x76,0x66,0x60,0x60,0x60,0xf0,0x0,0x0,0x0,0x0,
    // s
    0x0,0x0,0x0,0x0,0x0,0x7c,0xc6,0x60,0x38,0xc,0xc6,0x7c,0x0,0x0,0x0,0x0,
    // t
    0x0,0x0,0x10,0x30,0x30,0xfc,0x30,0x30,0x30,0x30,0x36,0x1c,0x0,0x0,0x0,0x0,
    // u
    0x0,0x0,0x0,0x0,0x0,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x76,0x0,0x0,0x0,0x0,
    // v
    0x0,0x0,0x0,0x0,0x0,0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x0,0x0,0x0,0x0,
    // w
    0x0,0x0,0x0,0x0,0x0,0xc6,0xc6,0xd6,0xd6,0xd6,0xfe,0x6c,0x0,0x0,0x0,0x0,
    // x
    0x0,0x0,0x0,0x0,0x0,0xc6,0x6c,0x38,0x38,0x38,0x6c,0xc6,0x0,0x0,0x0,0x0,
    // y
    0x0,0x0,0x0,0x0,0x0,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0x7e,0x6,0xc,0xf8,0x0,
    // z
    0x0,0x0,0x0,0x0,0x0,0xfe,0xcc,0x18,0x30,0x60,0xc6,0xfe,0x0,0x0,0x0,0x0,
    // {
        0x0,0x0,0xe,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x18,0xe,0x0,0x0,0x0,0x0,
    // |
    0x0,0x0,0x18,0x18,0x18,0x18,0x0,0x18,0x18,0x18,0x18,0x18,0x0,0x0,0x0,0x0,
    // }
    0x0,0x0,0x70,0x18,0x18,0x18,0xe,0x18,0x18,0x18,0x18,0x70,0x0,0x0,0x0,0x0,
    // ~
    0x0,0x0,0x76,0xdc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    };


    // Return a zoid list of the pixels in the character bitmap.  The list
    // is sorted t->b, l->r, and each element is square.  Arg d is the
    // "pixel" size.  The lower left corner is at x, y
    //
    Zlist *
    char_to_zlist(int c, int d, int x, int y)
    {
        if (c < '!' || c > '~')
            return (0);
        const unsigned char *map = an8X16font + (c - '!')*16;
        Zlist *zl0 = 0, *ze = 0;
        for (int i = 15; i >= 0; i--) {
            unsigned char ctmp = *map;
            unsigned char mask = 0x80;
            for (int j = 0; j < 8; j++) {
                if (mask & ctmp) {
                    Zoid Z(x + j*d, x + (j+1)*d, y + i*d, x + j*d, x + (j+1)*d,
                        y + (i+1)*d);
                    if (!zl0)
                        zl0 = ze = new Zlist(&Z);
                    else {
                        ze->next = new Zlist(&Z);
                        ze = ze->next;
                    }
                }
                mask >>= 1;
            }
            map++;
        }
        return (zl0);
    }
}


//----------------
// Ghost Rendering

// Function to render the logo text string or image during placement.
//
void
LabelState::show_logo(WindowDesc *wdesc, char *label, int x, int y,
    int width, int height, int xform, GRvecFont *ft)
{
    if (!label || !*label)
        return;
    if (!wdesc->Wdraw())
        return;

    DSP()->LabelSetTransform(xform, SLtrueOrient, &x, &y, &width, &height);

    wdesc->Wdraw()->SetFillpattern(0);
    wdesc->Wdraw()->SetLinestyle(0);
    if (label::is_xpm(label)) {
        int lwid, lhei;
        if (label::xpm_size(label, &lwid, &lhei)) {
            int pw = width / lwid;
            int xos = pw;
            int yos = 0;
            if (!Plist || Xos != xos || Yos != yos || Pwidth != pw) {
                PolyList::destroy(Plist);
                Zlist *z = label::xpm_to_zlist(label, pw, xos, yos);
                Plist = Zlist::to_poly_list(z);
                Xos = xos;
                Yos = yos;
                Pwidth = pw;
            }
            for (PolyList *p = Plist; p; p = p->next)
                Gst()->ShowGhostPath(p->po.points, p->po.numpts);
        }
    }
    else if (LogoMode != LogoNormal) {
        int lwid, lhei, numlines;
        bool pretty = (LogoMode == LogoPretty);
        label::manh_text_extent(label, &lwid, &lhei, &numlines, pretty);
        int pw = width / lwid;
        int xos = pw;
        int yos = 0;
        if (numlines > 1)
            yos = (lhei - lhei/numlines)*pw;

        if (!Plist || Xos != xos || Yos != yos || Pwidth != pw) {
            PolyList::destroy(Plist);
            Plist = 0;

            // To avoid spurious rendering, hide this from the
            // polytext functions, which may set variables which
            // trigger an update call.
            LabelCmd = 0;
            Plist = label::string_to_polys(label, pw, xos, yos, lwid,
                pretty);
            LabelCmd = this;
            Xos = xos;
            Yos = yos;
            Pwidth = pw;
        }
        for (PolyList *p = Plist; p; p = p->next)
            Gst()->ShowGhostPath(p->po.points, p->po.numpts);
    }
    else {
        int lwid, lhei, numlines;
        ft->textExtent(label, &lwid, &lhei, &numlines);
        int pw = width / lwid;
        int xos = ft->xoffset(label, xform, pw, lwid);
        int yos = numlines > 1 ? (numlines-1)*pw*ft->cellHeight() : 0;

        for (char *s = label; *s; s++) {
            GRvecFont::Character *cp = LogoFT.entry(*s);
            if (cp) {
                for (int i = 0; i < cp->numstroke; i++) {
                    GRvecFont::Cstroke *stroke = &cp->stroke[i];
                    Point *pts = new Point[stroke->numpts];
                    for (int j = 0; j < stroke->numpts; j++) {
                        pts[j].set(
                            pw*(stroke->cp[j].x - cp->ofst) + xos,
                            pw*(LogoFT.cellHeight() - 1 -
                                stroke->cp[j].y) + yos);
                    }
                    Wire wire;
                    wire.points = pts;
                    wire.numpts = stroke->numpts;
                    wire.set_wire_style(Style);
                    wire.set_wire_width(
                        quantize((int)(pw*PWmult[PathWidth-1])));
                    EGst()->showGhostWire(&wire);
                }
                xos += pw*cp->width;
            }
            else if (*s == '\n') {
                xos = ft->xoffset(s+1, xform, pw, lwid);
                yos -= pw*LogoFT.cellHeight();
            }
            else
                xos += pw*LogoFT.cellWidth();
        }
    }
    DSP()->TPop();
}


void
cEditGhost::showGhostLabel(int x, int y, int, int)
{
    if (!LabelCmd)
        return;
    int xform = LabelCmd->current_xform();
    GRvecFont *ft = LabelCmd->DoLogo ? &LogoFT : &FT;
    char *text = hyList::string(LabelCmd->Text, HYcvPlain, false);
    int width, height, numlines;
    if (LabelCmd->DoLogo) {
        if (label::is_xpm(text)) {
            if (!label::xpm_size(text, &width, &height))
                return;
            numlines = 1;
        }
        else if (LabelCmd->LogoMode == LogoNormal)
            ft->textExtent(text, &width, &height, &numlines);
        else if (LabelCmd->LogoMode == LogoPretty)
            label::manh_text_extent(text, &width, &height, &numlines, true);
        else
            label::manh_text_extent(text, &width, &height, &numlines, false);
    }
    else
        ft->textExtent(text, &width, &height, &numlines);

    if (DSP()->CurMode() == Physical) {
        if (LabelCmd->DoLogo) {
            int pw = LogoPixelSize;
            if (!pw)
                pw = quantize(
                    INTERNAL_UNITS(DSP()->PhysCharWidth()/ft->cellWidth()));
            width *= pw;
            height *= pw;
        }
        else {
            width = INTERNAL_UNITS(
                (DSP()->PhysCharWidth() * width)/ft->cellWidth());
            height = INTERNAL_UNITS(
                (DSP()->PhysCharHeight() * height)/ft->cellHeight());
        }
    }
    else {
        width = ELEC_INTERNAL_UNITS(
            (DSP()->ElecCharWidth() * width)/ft->cellWidth());
        height = ELEC_INTERNAL_UNITS(
            (DSP()->ElecCharHeight() * height)/ft->cellHeight());
    }

    WindowDesc *wdesc;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wdesc = wgen.next()) != 0) {
        if (!wdesc->IsSimilar(DSP()->MainWdesc()))
            continue;
        if (LabelCmd->DoLogo)
            LabelCmd->show_logo(wdesc, text, x, y, width, height, xform, ft);
        else {
            wdesc->ShowLabel(text, x, y, width, height, xform, ft);
            if (LabelCmd->Text->is_label_script()) {
                // Draw an outline around the label.
                wdesc->ShowLabelOutline(x, y, width, height, xform);
            }
        }
    }
    delete [] text;
}

