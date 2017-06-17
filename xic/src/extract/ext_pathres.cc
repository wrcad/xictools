
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
 $Id: ext_pathres.cc,v 5.16 2015/01/09 20:17:24 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_pathfinder.h"
#include "ext_rlsolver.h"
#include "geo_zgroup.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "promptline.h"
#include "menu.h"
#include "layertab.h"


//------------------------------------------------------------------------
// Terminal Editor and Net Resistance Extraction
//------------------------------------------------------------------------

namespace {
    namespace ext_pathres {
        struct TermState : public CmdState
        {
            TermState(const char*, const char*);
            virtual ~TermState();

            void setCaller(GRobject c)  { caller = c; }

            void b1down();
            void b1up();
            void esc();

        private:
            GRobject caller;
        };

        TermState *TermCmd;
    }
}

using namespace ext_pathres;


TermState::TermState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    caller = 0;
}


TermState::~TermState()
{
    TermCmd = 0;
}


void
TermState::b1down()
{
    cEventHdlr::sel_b1down();
}


void
TermState::b1up()
{
    BBox AOI;
    bool ret = cEventHdlr::sel_b1up(&AOI, 0, B1UP_NOSEL);
    if (!ret)
        return;

    WindowDesc *wdesc = EV()->ButtonWin();
    if (!wdesc)
        wdesc = DSP()->MainWdesc();

    int delcnt = 0;
    EX()->showTerminals(0, ERASE);

    Blist *tp = 0, *tn;
    for (Blist *t = EX()->terminals(); t; t = tn) {
        tn = t->next;
        if (t->BB.intersect(&AOI, false)) {
            if (!tp)
                EX()->setTerminals(tn);
            else
                tp->next = tn;
            delete t;
            delcnt++;
            continue;
        }
        tp = t;
    }
    int delta = (int)(10.0/wdesc->Ratio());
    if (!delcnt && AOI.width() >= delta && AOI.height() >= delta) {
        if (!EX()->terminals())
            EX()->setTerminals(new Blist(&AOI, 0));
        else {
            Blist *b = EX()->terminals();
            while (b->next)
                b = b->next;
            b->next = new Blist(&AOI, 0);
        }
    }

    Blist *t = EX()->terminals();
    if (t && t->next) {
        t = t->next;
        if (t->next) {
            PL()->ShowPrompt("Two terminals are already defined.");
            t->next->free();
            t->next = 0;
        }
    }

    EX()->showTerminals(0, DISPLAY);
    EX()->PopUpSelections(0, MODE_UPD);
}


void
TermState::esc()
{
    if (caller)
        Menu()->Deselect(caller);
    EV()->PopCallback(this);
    PL()->ErasePrompt();
    EX()->showTerminals(0, ERASE);
    Blist *bl = EX()->terminals();
    EX()->setTerminals(0);
    bl->free();
    EX()->PopUpSelections(0, MODE_UPD);
    delete this;
}


void
cExt::editTerminals(GRobject caller)
{
    if (TermCmd) {
        TermCmd->esc();
        return;
    }
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("Command available in physical mode only.");
        if (caller)
            Menu()->Deselect(caller);
        return;
    }

    TermCmd = new TermState("EDTERM", "xic:exsel#resis");
    TermCmd->setCaller(caller);
    if (!EV()->PushCallback(TermCmd)) {
        delete TermCmd;
        TermCmd = 0;
        return;
    }
    showTerminals(0, DISPLAY);
    PL()->ShowPrompt("Drag to define terminals.");
}


// Display terminal areas.
//
void
cExt::showTerminals(WindowDesc *wdesc, bool d_or_e)
{
    if (!ext_terminals)
        return;

    if (!wdesc) {
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0)
            showTerminals(wdesc, d_or_e);
        return;
    }

    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
        return;

    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(d_or_e ? GRxHlite : GRxUnhlite);
    else {
        if (d_or_e)
            wdesc->Wdraw()->SetColor(
                DSP()->Color(HighlightingColor, Physical));
        else {
            int tnum = 0;
            for (Blist *t = ext_terminals; t; t = t->next) {
                wdesc->Redisplay(&t->BB);
                tnum++;
            }
            return;
        }
    }

    int tnum = 0;
    for (Blist *t = ext_terminals; t; t = t->next) {
        wdesc->ShowLineW(t->BB.left, t->BB.bottom, t->BB.left, t->BB.top);
        wdesc->ShowLineW(t->BB.left, t->BB.top, t->BB.right, t->BB.top);
        wdesc->ShowLineW(t->BB.right, t->BB.top, t->BB.right, t->BB.bottom);
        wdesc->ShowLineW(t->BB.right, t->BB.bottom, t->BB.left, t->BB.bottom);
        if (tnum == 0)
            // Show corner diag for reference terminal.
            wdesc->ShowLineW(t->BB.left,
                (t->BB.bottom + 3*t->BB.top)/4,
                (3*t->BB.left + t->BB.right)/4, t->BB.top);
        else if (tnum > 1) {
            // Show cross for bogus terminals
            wdesc->ShowLineW(t->BB.left, t->BB.bottom, t->BB.right,
                t->BB.top);
            wdesc->ShowLineW(t->BB.left, t->BB.top, t->BB.right,
                t->BB.bottom);
        }
        tnum++;
    }

    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(GRxNone);
    else if (LT()->CurLayer())
        wdesc->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
}


bool
cExt::extractNetResistance(double **pvals, int *psz, const char *spicefile)
{
    if (pvals)
        *pvals = 0;
    if (psz)
        *psz = 0;

    pathfinder *pf = EX()->pathFinder(cExt::PFget);
    if (!pf || pf->is_empty()) {
        Errs()->add_error("extractNetResistance: no current path.");
        return (false);
    }

    Zgroup zg;
    zg.list = new Zlist*[2];

    Blist *bl = EX()->terminals();
    while (bl) {
        zg.list[zg.num] = new Zlist(&bl->BB);
        zg.num++;
        bl = bl->next;
        if (zg.num == 2)
            break;
    }
    if (zg.num < 2) {
        Errs()->add_error("extractNetResistance: too few terminals.");
        return (false);
    }

    CDol *ol = pf->get_object_list()->to_ol();
    if (!ol) {
        Errs()->add_error("extractNetResistance: empty object list.");
        return (false);
    }

    MRsolver r;
    bool ret = r.load_path(ol);  // frees ol and objects
    if (!ret)
        return (false);
    for (int i = 0; i < zg.num; i++) {
        ret = r.add_terminal(zg.list[i]);
        if (!ret)
            return (false);
    }

    ret = r.find_vias();
    if (!ret)
        return (false);

    ret = r.solve_elements();
    if (!ret)
        return (false);

    if (spicefile && *spicefile) {
        FILE *fp = fopen(spicefile, "w");
        if (!fp) {
            Errs()->sys_error("fopen");
            return (false);
        }
        ret = r.write_spice(fp);
        fclose(fp);
        if (!ret)
            return (false);
    }

    if (pvals && psz) {
        int sz;
        float *mat;
        ret = r.solve(&sz, &mat);
        if (!ret)
            return (false);

        int nsz = (sz == 1 ? 1 : (sz*(sz-1))/2);
        double *vals = new double[nsz];

        if (sz == 1)
            vals[0] = mat[0];
        else {
            double *rp = vals;
            for (int i = 0; i < sz; i++) {
                for (int j = i+1; j < sz; j++)
                    *rp++ = -1.0/mat[i*sz + j];
            }
        }
        delete [] mat;
        *psz = nsz;
        *pvals = vals;
    }
    return (true);
}

