
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
 $Id: sced_plot.cc,v 5.22 2016/02/21 18:49:43 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "sced_spiceipc.h"
#include "dsp_inlines.h"
#include "menu.h"
#include "promptline.h"
#include "events.h"


/***********************************************************************
 *
 * Functions for displaying spice output graphically.
 *
 ***********************************************************************/

namespace {
    void check_list(hyList**);
    void remove_from_list(hyList**, hyList*);
}


// Menu command to plot variables.  Pointing at nodes or other hot spots
// adds terms to plot string shown.  Hitting Enter brings up plot.
//
void
cSced::showOutputExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell()->isEmpty()) {
        PL()->ShowPrompt("No electrical data found in current cell.");
        return;
    }
    if (CurCell()->cellname() != DSP()->TopCellName()) {
        PL()->ShowPrompt("Pop to top level first.");
        return;
    }

    connectAll(true);
    sc_doing_plot = true;
    for (;;) {
        check_list(&sc_plot_hpr_list);
        check_list(&sc_iplot_hpr_list);
        hyList *htmp = PL()->EditHypertextPrompt("plot ", sc_plot_hpr_list ?
            sc_plot_hpr_list : sc_iplot_hpr_list, false);
        if (htmp) {
            char *s = hyList::string(htmp, HYcvPlain, false);
            if (s) {
                hyList::destroy(sc_plot_hpr_list);
                sc_plot_hpr_list = htmp;
                bool ret = spif()->ExecPlot(s);
                delete [] s;
                if (!ret)
                    break;
            }
        }
        else
            break;
    }
    sc_doing_plot = false;
}


// Menu command to set up or unset an iplot.  If setting, the iplot string
// is shown for editing.  Pressing Enter completes editing and enables iplot.
//
void
cSced::setDoIplotExec(CmdDesc *cmd)
{
    Deselector ds(cmd);
    if (!XM()->CheckCurMode(Electrical))
        return;
    if (!XM()->CheckCurCell(false, false, Electrical))
        return;
    if (CurCell()->isEmpty()) {
        PL()->ShowPrompt("No electrical data found in current cell.");
        return;
    }

    sc_doing_iplot = cmd && Menu()->GetStatus(cmd->caller);
    if (!sc_doing_iplot) {
        spif()->ClearIplot();
        sc_iplot_status_changed = false;
        PL()->ShowPrompt("No plotting during simulation.");
    }
    else {
        if (CurCell()->cellname() != DSP()->TopCellName()) {
            PL()->ShowPrompt("Pop to top level first.");
            sc_doing_iplot = false;
            return;
        }
        connectAll(true);
        sc_doing_plot = true;
        check_list(&sc_plot_hpr_list);
        check_list(&sc_iplot_hpr_list);
        hyList *htmp = PL()->EditHypertextPrompt("iplot ", sc_iplot_hpr_list ?
            sc_iplot_hpr_list : sc_plot_hpr_list, false);
        sc_doing_plot = false;
        if (htmp) {
            char *s = hyList::string(htmp, HYcvPlain, false);
            if (s) {
                char *estr = findPlotExpressions(s);
                delete [] s;
                if (estr && *estr) {
                    hyList::destroy(sc_iplot_hpr_list);
                    sc_iplot_hpr_list = htmp;
                    delete [] estr;
                    PL()->ShowPrompt("Will generate plot while simulating.");
                    sc_iplot_status_changed = true;
                    ds.clear();
                    return;
                }
                PL()->ShowPrompt("No plotting during simulation.");
                delete [] estr;
                hyList::destroy(sc_iplot_hpr_list);
                sc_iplot_hpr_list = 0;
            }
            hyList::destroy(htmp);
        }
        // no string entered, abort
        sc_doing_iplot = false;
    }
}


// Returns true if hent is a plot or iplot reference,
// in either case it is deleted.
//
int
cSced::deletePlotRef(hyEnt *hent)
{
    if (!hent)
        return (false);
    hyList *h0 = 0, *h;
    for (h = sc_plot_hpr_list; h; h0 = h,h = h->next()) {
        if (h->hent() == hent) {
            if (!h0)
                sc_plot_hpr_list = h->next();
            else
                h0->set_next(h->next());
            break;
        }
    }
    if (!h) {
        h0 = 0;
        for (h = sc_iplot_hpr_list; h; h0 = h,h = h->next()) {
            if (h->hent() == hent) {
                if (!h0)
                    sc_iplot_hpr_list = h->next();
                else
                    h0->set_next(h->next());
                sc_iplot_status_changed = true;
                break;
            }
        }
    }
    if (h) {
        delete hent;
        return (true);
    }
    return (false);
}


// This is called from the hypertext editor when the plot command is
// active.  It identifies the trace index that contains each hypertext
// reference, and sets the corresponding mark color.  The mark color
// will match the trace color in the WRspice plot if colors have not
// been altered by the user - both Xic and WRspice start with the same
// defaults.
//
void
cSced::setPlotMarkColors()
{
    hyList *curlist = PL()->List(false);
    if (!curlist)
        return;
    char *plstr = hyList::string(curlist, HYcvPlain, false);
    char *expr_str0 = findPlotExpressions(plstr);
    delete [] plstr;
    const char *expr_str = expr_str0;

    char *expr = expr_str ? lstring::getqtok(&expr_str) : 0;
    int tok_ix = 0;
    int clr_ix = 0;

    for (hyList *h = curlist; h; h = h->next()) {
        if (!h->hent())
            continue;
        if (h->ref_type() != HLrefNode && h->ref_type() != HLrefBranch)
            continue;
        tok_ix++;
        if (!expr) {
            DSP()->SetPlotMarkColor(tok_ix, -1);
            continue;
        }
        char *estr = hyList::get_entry_string(h);
        if (!estr) {
            DSP()->SetPlotMarkColor(tok_ix, -1);
            continue;
        }
        if (!strstr(expr, estr)) {
            char *nxpr;
            const char *s = expr_str;
            int ntok = 0;
            bool found = false;
            while ((nxpr = lstring::getqtok(&s)) != 0) {
                ntok++;
                if (strstr(nxpr, estr)) {
                    delete [] expr;
                    expr = nxpr;
                    expr_str = s;
                    clr_ix += ntok;
                    found = true;
                    break;
                }
                delete [] nxpr;
            }
            if (!found) {
                // Hmmm, shouldn't happen.
                delete [] estr;
                DSP()->SetPlotMarkColor(tok_ix, -1);
                continue;
            }
        }
        delete [] estr;
        DSP()->SetPlotMarkColor(tok_ix, clr_ix);
    }
    delete [] expr;
    delete [] expr_str0;
    hyList::destroy(curlist);
}


// Clear all plot and iplot references.
//
void
cSced::clearPlots()
{
    if (!DSP()->CurCellName())
        return;
    if (sc_plot_hpr_list) {
        hyList::destroy(sc_plot_hpr_list);
        sc_plot_hpr_list = 0;
    }
    if (sc_iplot_hpr_list) {
        hyList::destroy(sc_iplot_hpr_list);
        sc_iplot_hpr_list = 0;
        sc_iplot_status_changed = true;
    }
    Menu()->MenuButtonSet("side", "iplot", false);
}


// Return the plot string.  If ascii is true, return as ascii hypertext,
// otherwise as plain text.
//
char *
cSced::getPlotCmd(bool ascii)
{
    if (!sc_plot_hpr_list)
        return (0);
    check_list(&sc_plot_hpr_list);
    return (hyList::string(sc_plot_hpr_list, ascii ? HYcvAscii : HYcvPlain,
        false));
}


// Set the current plot list, input is ascii format hypertext.
//
void
cSced::setPlotCmd(const char *cmd)
{
    CDs *cursde = CurCell(Electrical);
    if (cursde) {
        if (sc_plot_hpr_list)
            hyList::destroy(sc_plot_hpr_list);
        sc_plot_hpr_list = new hyList(cursde, cmd, HYcvAscii);
    }
}


// Return the iplot string.  If ascii is true, return as ascii hypertext,
// otherwise as plain text.
//
char *
cSced::getIplotCmd(bool ascii)
{
    if (!sc_iplot_hpr_list)
        return (0);
    check_list(&sc_iplot_hpr_list);
    return (hyList::string(sc_iplot_hpr_list, ascii ? HYcvAscii : HYcvPlain,
        false));
}


// Set the current iplot list, input is ascii format hypertext.
//
void
cSced::setIplotCmd(const char *cmd)
{
    CDs *cursde = CurCell(Electrical);
    if (cursde) {
        if (sc_iplot_hpr_list)
            hyList::destroy(sc_iplot_hpr_list);
        sc_iplot_hpr_list = new hyList(cursde, cmd, HYcvAscii);
    }
}


// Return the current plot list.
//
hyList *
cSced::getPlotList()
{
    return (sc_plot_hpr_list);
}


// Return the current iplot list.
//
hyList *
cSced::getIplotList()
{
    return (sc_iplot_hpr_list);
}


namespace {
    // Remove and free any bogus references.
    //
    void
    check_list(hyList **list)
    {
        hyList *hn;
        for (hyList *h = *list; h; h = hn) {
            hn = h->next();
            if (h->ref_type() == HLrefText)
                continue;
            if (h->hent() && h->hent()->ref_type() != HYrefBogus) {
                char *s = hyList::get_entry_string(h);
                if (s) {
                    delete [] s;
                    continue;
                }
            }
            remove_from_list(list, h);
        }
    }


    // Remove hx from the list, and also the preceding space char if
    // any if hx is hypertext, and free the removed elements.
    //
    void
    remove_from_list(hyList **list, hyList *hx)
    {
        if (!*list)
            return;
        hyList *hprev = 0, *hprev2 = 0;
        for (hyList *h = *list; h; h = h->next()) {
            if (h == hx) {
                if (h->ref_type() == HLrefText) {
                    if (hprev)
                        hprev->set_next(h->next());
                    else
                        *list = h->next();
                    delete h;
                    return;
                }
                if (hprev && hprev->ref_type() == HLrefText) {
                    if (!hprev->text() ||
                            (isspace(hprev->text()[0]) && !hprev->text()[1])) {
                        if (hprev2)
                            hprev2->set_next(h->next());
                        else
                            *list = h->next();
                        delete hprev;
                        delete h;
                        return;
                    }
                    int len = strlen(hprev->text());
                    if (isspace(hprev->text()[len-1]))
                        hprev->etext()[len-1] = 0;
                }
                if (hprev)
                    hprev->set_next(h->next());
                else
                    *list = h->next();
                delete h;
                return;
            }
            hprev2 = hprev;
            hprev = h;
        }
    }
}

